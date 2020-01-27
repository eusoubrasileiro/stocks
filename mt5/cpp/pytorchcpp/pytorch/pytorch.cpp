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
th::Device device = deviceCPU;

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
    thfdcoefs.to(device);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int FracDifApply(double signal[], int size, double output[]){
  th::Tensor thdata = th::from_blob(signal, { 1, 1, size}, dtype64_option).clone();
  // to GPU or not
  thdata.to(device);
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
              device = th::Device(th::kCUDA);
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
void sadf(float *signal, int n, int maxw, int minw, int p, float gpumem_gb=2.0, bool verbose=false){
    th::NoGradGuard guard; // same as with torch.no_grad(): block
  // fastest version
  //     - assembly rows of OLS problem using entire input data
  //     - send batchs of 1GB adfs tests to GPU until entire
  //     sadf is calculated, last batch might (should be smaller)
    auto dtype_option = dtype32_option;
    auto nobs = n-p-1; // data used for regression
    auto X = th::zeros({n-p-1, 3+p}, dtype_option);
    auto y = th::from_blob(signal, {n}, dtype_option);
       
    auto diffilter = th::tensor({-1, 1}, dtype_option).view({ 1, 1, 2 }); // first difference filter   
//    diffilter[0][0][0] = -1;
    auto dy = th::conv1d(y.view({ 1, 1, -1 }), diffilter).view({ -1 });
    y = y.view({ -1 });
    auto z = dy.slice(0, p).clone();

    //dtype_option.device(th::kCUDA);
    //dtype_option.device(th::kCPU);

    // fill in first 3 columns, drit, trend, reg. data
    // acessors or tensor[i][j].item<int>()
    auto ay = y.accessor<float, 1>();
    auto aX = X.accessor<float, 2>(); //index_fill(0, { 1 }, 1); // drift
    auto ady = dy.accessor<float, 1>();
    for (auto i = 0; i < nobs; i++) {
        aX[i][0] = 1; // drift term
        aX[i][1] = p + 1 + i; // deterministic trend
        aX[i][2] = ay[p+i];  // regression data(nobs)
    }
    // fill in other columns, start at third 
    for (auto j = 0; j < nobs; j++)
        for (auto i = 1; i < p + 1; i++)
            aX[j][2 + i] = ady[p - i + j];
            //X[:, 2+i] = dy[p-i:-i]

    // 1 sadf point requires at least GB (only matrix storage)
    auto adfs_count = maxw - minw; // number of adfs for one sadf t
    auto nadf = maxw; // data used is (maximum - first adf)
    auto nobsadf = nadf - p - 1; // number of observations (maximum - first adf)
    auto xsize = (nobsadf * (3 + p)) * 4; // 4 bytes float32
    auto sadft_GB = float( xsize * adfs_count / GIGABytes); // storage for 1 sadf point
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
    auto Xbtc = th::zeros({ batch_size*adfs_count, nobsadf, (3 + p) }, dtype_option.device(device));
    auto zbtc = th::zeros({ batch_size*adfs_count, nobsadf }, dtype_option.device(device));
    auto nobtc = th::zeros({ batch_size*adfs_count }, dtype_option.device(device));

    // result (allways on CPU - faster)
    auto sadf = th::zeros({ nsadft }, dtype_option.device(deviceCPU));

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
                //                 Xbt[j, k, :k, :] = 0
                //                 zbt[j, k, :k] = 0
                //nobt[j][k] = float(nobsadf - k - (p + 3));                
                // sadf loop until minw, every matrix is smaller than the previous
                // zeroing i lines, observations are becomming less also
                Xbts.select(0, k).narrow(0, 0, k).fill_(0);
                zbts.select(0, k).narrow(0, 0, k).fill_(0);
                anobt[j][k] = float(nobsadf - k - (p + 3));
            }
        }


    }

}

//             # sadf loop until minw, every matrix is smaller than the previous
//             # zeroing i lines, observations are becomming less also
//             for k in range(adfs_count): # each is smaller than previous
//                 Xbt[j, k, :k, :] = 0
//                 zbt[j, k, :k] = 0
//                 nobt[j, k] = float(nobsadf-k-(p+3))
//
//             t = t + 1
//         # TO CUDA
//         Xbt_[:] = Xbt.view(batch_size*adfs_count, nobsadf, (3+p))[:]
//         zbt_[:] = zbt.view(batch_size*adfs_count, nobsadf, 1)[:]
//         nobt_[:] = nobt.view(batch_size*adfs_count)[:]
//         # remove unsqueeze by 1 add on view
//         #zbt_ = zbt_.unsqueeze(dim=-1) # additonal dim for matrix*vector mult.
//         Xt = Xbt_.transpose(dim0=1, dim1=-1)
//         Gi = th.inverse(th.bmm(Xt, Xbt_)) # ( X^T . X ) ^-1
//         Bhat = th.bmm(Gi, th.bmm(Xt, zbt_))
//         er = zbt_ - th.bmm(Xbt_, Bhat)
//         Bhat = Bhat.squeeze()
//         s2 = (er*er).sum(1).squeeze()/nobt_
//         adfstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])
//
//         # adfstats = torch_bmadf(Xbt.view(batch_size*adfs_count, nobsadf, (3+p)),
//         #                      zbt.view(batch_size*adfs_count, nobsadf),
//         #                      nobt.view(batch_size*adfs_count))
//
//         sadf[(t-batch_size):t] = th.max(adfstats.view(batch_size, adfs_count), -1)[0]
//
//     # last fraction of a batch
//     for j in range(lst_batch_size): # assembly batch_size sadf'ts matrixes
//         Xm[:] = X[t:t+nobsadf] # master X for this sadft - biggest adf OLS X matrix
//         zm[:]  = z[t:t+nobsadf] # master Z for this sadft (biggest adf OLS independent term)
//
//         Xbt[j, :, :, :] = Xm.repeat(adfs_count, 1).view(adfs_count, nobsadf, (3+p))
//         zbt[j, :, :] = zm.repeat(adfs_count, 1).view(adfs_count, nobsadf)
//
//         # sadf loop until minw, every matrix is smaller than the previous
//         # zeroing i lines, observations are becomming less also
//         for k in range(adfs_count): # each is smaller than previous
//             Xbt[j, k, :k, :] = 0
//             zbt[j, k, :k] = 0
//             nobt[j, k] = float(nobsadf-k-(p+3))
//
//         t = t + 1
//         # TO CUDA
//         Xbt_[:lst_batch_size*adfs_count] = Xbt[:lst_batch_size,:, :, :].view(
//                                      lst_batch_size*adfs_count, nobsadf, (3+p))[:]
//         zbt_[:lst_batch_size*adfs_count] = zbt[:lst_batch_size,:, :].view(
//                                      lst_batch_size*adfs_count, nobsadf, 1)[:]
//         nobt_[:lst_batch_size*adfs_count] = nobt[:lst_batch_size,:].view(lst_batch_size*adfs_count)[:]
//         Xbt_ = Xbt_[:lst_batch_size*adfs_count]
//         zbt_ = zbt_[:lst_batch_size*adfs_count]
//         nobt_ = nobt_[:lst_batch_size*adfs_count]
//         Xt = Xbt_.transpose(dim0=1, dim1=-1)
//         Gi = th.inverse(th.bmm(Xt, Xbt_)) # ( X^T . X ) ^-1
//         Bhat = th.bmm(Gi, th.bmm(Xt, zbt_))
//         er = zbt_ - th.bmm(Xbt_, Bhat)
//         Bhat = Bhat.squeeze()
//         s2 = (er*er).sum(1).squeeze()/nobt_
//         adfstats = Bhat[:, 2]/th.sqrt(s2*Gi[:, 2,2])
//
//     # adfstats = torch_bmadf(Xbt[:lst_batch_size,:, :, :].view(
//     #                             lst_batch_size*adfs_count, nobsadf, (3+p)),
//     #                      zbt[:lst_batch_size,:, :].view(
//     #                             lst_batch_size*adfs_count, nobsadf),
//     #                      nobt[:lst_batch_size,:].view(lst_batch_size*adfs_count))
//
//     sadf[(t-lst_batch_size):t] = th.max(adfstats.view(lst_batch_size, adfs_count), -1)[0]
//
//     return sadf.to('cpu').data.numpy()
