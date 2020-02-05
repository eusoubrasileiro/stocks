#pragma warning (disable : 4146)
#include <iostream>
#include <torch\torch.h>
#define PYTORCHCPP_DLL
#include "pytorchcpp.h"

namespace th = torch;

auto dtype32_option = th::TensorOptions().dtype(th::kFloat32).requires_grad(false);
auto dtype64_option = th::TensorOptions().dtype(th::kFloat64).requires_grad(false);

th::Tensor thfdcoefs;
th::Device deviceCPU = th::Device(th::kCPU);
th::Device deviceifGPU = deviceCPU;

// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setFracDifCoefs(double d, int size){
    double *w = new double[size];
    w[0] = 1.;
    for(int k=1; k<size; k++)
        w[k]=-w[k-1]/k*(d-k+1);
    std::reverse(w, w+size);
    thfdcoefs = th::from_blob(w, {1, 1, size}, dtype64_option);
	// to GPU or not
    thfdcoefs.to(deviceifGPU);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int FracDifApply(double signal[], int size, double output[]){
  th::Tensor thdata = th::from_blob(signal, { 1, 1, size}, dtype64_option).clone();
  // to GPU or not
  thdata.to(deviceifGPU);
  th::Tensor thresult = th::conv1d(thdata, thfdcoefs).reshape({ -1 });
  // back to CPU
  thresult = thresult.to(deviceCPU);
  // double array output
  double* ptr_data = thresult.data_ptr<double>();
  int outsize = thresult.size(0);
  memcpy(output, ptr_data, sizeof(double) * outsize);

  return outsize;
}




BOOL APIENTRY DllMain( HMODULE hModule,
                       DWORD  ul_reason_for_call,
                       LPVOID lpReserved
                     )
{
    switch (ul_reason_for_call)
    {
        case DLL_PROCESS_ATTACH:
            if (th::cuda::is_available()) {
              //std::cout << "CUDA is available! Running on GPU." << std::endl;
              deviceifGPU = th::Device(th::kCUDA);
            }
            break;
        case DLL_PROCESS_DETACH:
            // detach from process
            break;
        case DLL_THREAD_ATTACH:
            // attach to thread
            break;
        case DLL_THREAD_DETACH:
            // detach from thread
            break;
    }
    return TRUE; // succesful
}

#define GIGABytes 1073741824.0

// supremum augmented dickey fuller test SADF
// expands backward many adfs for each point
// using minw window size and maxw as maximum backward size
int sadf(float *signal, float *out, int n, int maxw, int p, float gpumem_gb=2.0, bool verbose=false){
    th::NoGradGuard guard; // same as with torch.no_grad(): block
  // fastest version
  //     - assembly rows of OLS problem using entire input data
  //     - send batchs of 1GB adfs tests to GPU until entire
  //     sadf is calculated, last batch might (should be smaller)

    int minw = (2*p + 4)*1.5; // to avoid many problems of bad conditionning of OLS matrix

    if (minw > maxw){
        if (verbose) std::cout << "error minw > maxw " << std::endl;
        return 0;
    }

    auto dtype_option = dtype32_option;
    auto nobs = n-p-1; // data used for regression
    auto X = th::zeros({nobs, 3+p}, dtype_option);
    auto y = th::from_blob(signal, {n}, dtype_option);

    auto diffilter = th::tensor({-1, 1}, dtype_option).view({ 1, 1, 2 }); // first difference filter
    auto dy = th::conv1d(y.view({ 1, 1, -1 }), diffilter).view({ -1 });
    auto z = dy.slice(0, p).clone();

    // fill in first 3 columns, drit, trend, reg. data
    // acessors or tensor[i][j].item<int>()
    auto ay = y.accessor<float, 1>();
    auto aX = X.accessor<float, 2>(); // drift
    auto ady = dy.accessor<float, 1>();
    for (auto i = 0; i < nobs; i++) {
        aX[i][0] = 1; // drift term
        aX[i][1] = p + 1 + i; // deterministic trend
        aX[i][2] = ay[p+i];  // regression data(nobs)
    }
    // fill in other columns, start at third
    for (auto j = 0; j < nobs; j++)
        for (auto i = 1; i < p + 1; i++) //X[:, 2+i] = dy[p-i:-i]
            aX[j][2 + i] = ady[p - i + j];

    // 1 sadf point requires at least GB (only matrix storage)
    auto adfs_count = maxw - minw; // number of adfs for one sadf t
    auto nadf = maxw; // data used is (maximum - first adf)
    auto nobsadf = nadf - p - 1; // number of observations (maximum - first adf)
    auto xsize = (nobsadf * (3 + p)) * 4; // 4 bytes float32
    auto sadft_GB = float( xsize * adfs_count / GIGABytes); // matrix storage for 1 sadf point
    auto nsadft = n - maxw; // number of sadf t's to calculate the entire SADF

    auto batch_size = 1; // number of sadft points to calculate at once
    if (sadft_GB < gpumem_gb) { // each batch will have at least 1GB in OLS matrixes
        batch_size = int(gpumem_gb / sadft_GB);
        batch_size = (batch_size > nsadft) ? nsadft : batch_size; // in case bigger than sadf
    }

    auto nbatchs = nsadft / batch_size; // number of batchs to sent to GPU
    auto lst_batch_size = nsadft - nbatchs * batch_size; //last batch of adfs(integer%)

    if (verbose) {
        std::cout << "adfs_count " << adfs_count << std::endl;
        std::cout << "sadft_GB " << sadft_GB << std::endl;
        std::cout << "batch_size " << batch_size << std::endl;
        std::cout << "nsadft " << nsadft << std::endl;
        std::cout << "nbatchs " << nbatchs << std::endl;
        std::cout << "lst_batch_size " << lst_batch_size << std::endl;
    }

    // master X for a sadft (biggest adf OLS X matrix)
    auto Xm = th::zeros({ nobsadf, 3 + p }, dtype_option.device(deviceCPU));
    // master Z for a sadft (biggest adf OLS independent term)
    auto zm = th::zeros({ nobsadf }, dtype_option.device(deviceCPU));

    // batch matrix, vector and observations for multiple adfs - assembly allways on CPU - faster
    auto Xbt = th::zeros({ batch_size, adfs_count, nobsadf, (3 + p) }, dtype_option.device(deviceCPU));
    auto zbt = th::zeros({ batch_size, adfs_count, nobsadf }, dtype_option.device(deviceCPU));
    auto nobt = th::zeros({ batch_size, adfs_count }, dtype_option.device(deviceCPU));

    //acessors
    auto aXbt = Xbt.accessor<float, 4>();
    auto azbt = zbt.accessor<float, 3>();
    auto anobt = nobt.accessor<float, 2>();

    // CUDA (if existent) - same as above to batch operation
    auto Xbtc = th::zeros({ batch_size*adfs_count, nobsadf, (3 + p) }, dtype_option.device(deviceifGPU));
    auto zbtc = th::zeros({ batch_size*adfs_count, nobsadf, 1}, dtype_option.device(deviceifGPU));
    auto nobtc = th::zeros({ batch_size*adfs_count }, dtype_option.device(deviceifGPU));

    // result
    auto sadf = th::zeros({ nsadft }, dtype_option.device(deviceifGPU));

    auto tline = 0; // start line for master main X OLS matrix/ z vector
    for (int i = 0; i < nbatchs; i++) {

        for (int j = 0; j < batch_size; j++) { // assembly batch_size sadf'ts matrixes
            Xm.copy_(X.narrow(0, tline, nobsadf)); //  master X for this sadft - biggest adf OLS X matrix
            zm.copy_(z.narrow(0, tline, nobsadf)); // master Z for this sadft (biggest adf OLS independent term)

            auto Xbts = Xbt.select(0, j);
            auto zbts = zbt.select(0, j);

            //Xbt[j, :, : , : ] = Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3 + p))
            //zbt[j, :, : ] = zm.repeat(adfs_count, 1).view(adfs_count, nobsadf)
            Xbts.copy_(Xm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf, (3 + p) }));
            zbts.copy_(zm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf }));

            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
                // Xbt[j, k, :k, :] = 0
                // zbt[j, k, :k] = 0
                // nobt[j][k] = float(nobsadf - k - (p + 3));
                // sadf loop until minw, every matrix is smaller than the previous
                // zeroing i lines, observations are becomming less also
                Xbts.select(0, k).narrow(0, 0, k).fill_(0);
                zbts.select(0, k).narrow(0, 0, k).fill_(0);
                anobt[j][k] = float(nobsadf - k - (p + 3));
            }
            tline++;
        }

        Xbtc.copy_(Xbt.view({ batch_size * adfs_count, nobsadf, (3 + p)}));
        zbtc.copy_(zbt.view({ batch_size * adfs_count, nobsadf, 1 }));
        nobtc.copy_(nobt.view({batch_size * adfs_count}));
        auto Xt = Xbtc.transpose(1, -1);
        auto Lower = th::cholesky(Xt.bmm(Xbtc));
        auto Gi = th::cholesky_solve(th::eye(p+3, dtype_option.device(deviceifGPU)), Lower); // (X ^ T.X) ^ -1
        auto Bhat = Gi.bmm(Xt.bmm(zbtc));
        auto er = zbtc - Xbtc.bmm(Bhat);
        Bhat = Bhat.squeeze();
        auto s2 = (er*er).sum(1).squeeze().div(nobtc);
        auto adfstats = Bhat.select(-1, 2).div(th::sqrt(Gi.select(-2, 2).select(-1, 2)*s2));
        sadf.narrow(0, tline - batch_size, batch_size).copy_(std::get<0>(adfstats.view({ batch_size, adfs_count }).max(-1)));

    }
    // last fraction of a batch (if exist)
    for (int j = 0; j < lst_batch_size; j++) { // assembly batch_size sadf'ts matrixes
        Xm.copy_(X.narrow(0, tline, nobsadf)); //  master X for this sadft - biggest adf OLS X matrix
        zm.copy_(z.narrow(0, tline, nobsadf)); // master Z for this sadft (biggest adf OLS independent term)

        auto Xbts = Xbt.select(0, j);
        auto zbts = zbt.select(0, j);

        Xbts.copy_(Xm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf, (3 + p) }));
        zbts.copy_(zm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf }));

        for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
            Xbts.select(0, k).narrow(0, 0, k).fill_(0);
            zbts.select(0, k).narrow(0, 0, k).fill_(0);
            anobt[j][k] = float(nobsadf - k - (p + 3));
        }
        tline++;
    }

    if (lst_batch_size > 0) {
        //TO CUDA
        Xbtc = Xbtc.narrow(0, 0, lst_batch_size * adfs_count);
        zbtc = zbtc.narrow(0, 0, lst_batch_size * adfs_count);
        nobtc = nobtc.narrow(0, 0, lst_batch_size * adfs_count);
        Xbtc.copy_(Xbt.narrow(0, 0, lst_batch_size).view({ lst_batch_size * adfs_count, nobsadf, (3 + p) }));
        zbtc.copy_(zbt.narrow(0, 0, lst_batch_size).view({ lst_batch_size * adfs_count, nobsadf, 1 }));
        nobtc.copy_(nobt.narrow(0, 0, lst_batch_size).view({ lst_batch_size * adfs_count }));
        auto Xt = Xbtc.transpose(1, -1);
        auto Lower = th::cholesky(Xt.bmm(Xbtc));
        auto Gi = th::cholesky_solve(th::eye(p+3, dtype_option.device(deviceifGPU)), Lower); // (X ^ T.X) ^ -1
        auto Bhat = Gi.bmm(Xt.bmm(zbtc));
        auto er = zbtc - Xbtc.bmm(Bhat);
        Bhat = Bhat.squeeze();
        auto s2 = (er * er).sum(1).squeeze().div(nobtc);
        auto adfstats = Bhat.select(-1, 2).div(th::sqrt(Gi.select(-2, 2).select(-1, 2) * s2));
        sadf.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(std::get<0>(adfstats.view({ lst_batch_size, adfs_count }).max(-1)));
    }

    sadf = sadf.to(deviceCPU);

    memcpy(out, sadf.data_ptr<float>(), sizeof(float)*nsadft);

    return nsadft;
}
