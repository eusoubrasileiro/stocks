#pragma once
#include "libpytorch.h"

// Very optimized SADF 
// Supremum Augmented Dickey Fuller test SADF
// or QSADF - quantile/percentile SADF
// using GPU to solve thousands of ADFs at once

// expands backward many adfs for each point
// using minw window size and maxw as maximum backward size
// fastest version
//     - assembly rows of OLS problem using entire input data
//     - send batchs of 1GB adfs tests to GPU until entire
//     sadf is calculated, last batch might (should be smaller)
// filling with zeros lines of matrix
// custom batch cholesky (Cholesky) was the only 
// solution to deal with nan on matrices due colinearity
//
// - initial version was SADF
//   - also give the lag of high ADF value for each SADFt point
// - QSADF version
//   - also give the dispersion of 
//


// Batch Cholesky based on
// https://www.pugetsystems.com/labs/hpc/PyTorch-for-Scientific-Computing---Quantum-Mechanics-Example-Part-3-Code-Optimizations---Batched-Matrix-Operations-Cholesky-Decomposition-and-Inverse-1225/
inline th::Tensor Cholesky(th::Tensor &A){
    th::NoGradGuard guard; // same as with torch.no_grad(): block

    auto L = th::zeros_like(A, dtype_option.device(deviceifGPU));
    auto s = th::zeros(A.size(0), dtype_option.device(deviceifGPU));

    for (int i = 0; i < A.size(-1); i++) {
        for (int j = 0; j < i + 1; j++) {
            s.fill_(0);
            for (int k = 0; k < j; k++) {
                s.add_(L.select(1, i).select(-1, k) * L.select(1, j).select(-1, k));
            }
            if (i == j)
                L.select(1, i).select(-1, j).copy_(th::sqrt(A.select(1, i).select(-1, i) - s));
            else
                L.select(1, i).select(-1, j).copy_(1.0 / L.select(1, j).select(-1, j) * (A.select(1, i).select(-1, j) - s));
        }
    }
  //
  // for i in range(A.shape[-1]):
  //     for j in range(i+1):
  //         s = 0.0
  //         for k in range(j):
  //             s = s + L[...,i,k] * L[...,j,k]
  //
  //         L[...,i,j] = th.sqrt(A[...,i,i] - s) if (i == j) else \
  //                   (1.0 / L[...,j,j] * (A[...,i,j] - s))
  // return L
    return L;
}


inline void PrintMatrices(th::Tensor& A, std::ofstream& file) {
    auto oncpu = A.to(deviceCPU); // in case is on GPU
    auto a = oncpu.accessor<float, 3>();

    char line[256];
    // file << "       time_msc       |   last   |  r.volume |  volume  " << std::endl;

    for (int i = 0; i < a.size(0); i++) { // number of matrices
        file << std::endl; // new matrix skip line
        for (int j = 0; j < a.size(1); j++) { // rows
            file << std::endl; // new row skip line
            for (int k = 0; k < a.size(2); k++) { // columns
                sprintf(line, "  %+5.5E", a[i][j][k]);
                file << std::string(line);
            }
        }
    }
}


// Calculating Index of Percentil Value
//size_t asize = data.size();
//auto n = (size_t)std::max(std::round(perc * asize + 0.5), 2.0);
//return data[n - 2];    
#define PERCENTIL_IDX(perc, len){((int64_t)std::max(std::round(perc * len + 0.5), 2.0))-2}

// parameters for QSADF or percentile SADF
#define QSADF_PERC 0.92 // percentile of 'max adf' value used to calculated percentile SADF 
#define QSADF_SPREAD 0.07 // to analyse dispersion of high ADF values


// outadfmaxidx: lag - which ADF backward lag gave the highest ADF
int sadf(float* signal, float* outsadf, float* outadfmaxdispersion, int n, int maxw, int minw, int order, bool drift, float gpumem_gb, bool verbose) {
    // working perfectly - only one GPU so only one thread can access it a time
    std::lock_guard<std::mutex> lock(m);

#ifdef  FILEDEBUG
    try {
#endif //  DEBUG
        th::NoGradGuard guard; // same as with torch.no_grad(): block

        // number of params of the AR model
        // 1. drift term (DC) - default
        // 2. deterministic trend (optional)
        // 3. gama or coeficient of y(t-1) - mandatory
        // 4. aditional order of AR model beyond t-2 and included
        int params = order + 2 + 1 * drift;
        // check for minimum number of data for the t statistical test
        // degrees of freedom cannot be zero -> neq_adf - params
        // auto neq_adf_min = minw - order - 1; //number of equations for shortest adf (minw)
        // neq_adf_min - params > 0 or minw - order - 1 - params > 0 -> minw > order + 1 + params
        if (minw <= order + 1 + params) {
            if (verbose) std::cout << "error minw must be > order + 1 + params: " << order + 1 + params << std::endl;
            return 0;
        }

        if (minw > maxw) {
            if (verbose) std::cout << "error minw > maxw " << std::endl;
            return 0;
        }

        auto neq = n - order - 1; // number of equations (lines)
        //auto nobs = n - order - 1; // data used for regression
        auto X = th::zeros({ neq, params }, dtype_option);
        auto y = th::from_blob(signal, { n }, dtype_option);
        // dont know why but conv1d is causing a SEH exception
        // replaced it by a vector subtraction
        //auto diffilter = th::tensor([-1, 1 }, dtype_option).view({ 1, 1, 2 }); // first difference filter
        auto dy = y.narrow(0, 1, n-1) - y.narrow(0, 0, n-1);
        //auto dy = th::conv1d(y.view({ 1, 1, -1 }), th::zeros(10)).view({ -1 });
        auto z = dy.slice(0, order).clone();

        // fill in first 3 columns, drit, trend, reg. data
        X.select(1, 0).fill_(1); // first column 1 - drift term
        if (drift) {
            X.select(1, 1).copy_(th::arange(order + 1, n)); // deterministic trend
            X.select(1, 2).copy_(y.narrow(0, order, neq)); // regression data(nobs)
        }
        else { // without drift term
            X.select(1, 1).copy_(y.narrow(0, order, neq)); // regression data(nobs)
        }
        // fill in other params columns, starting from the last column
        // ignoring the first param columns already filled
        for (auto i = params - 1, j = 0; i >= 2 + 1 * drift; i--, j++) //X[:, 2+i] = dy[p-i:-i]
            X.select(1, i).copy_(dy.narrow(0, j, neq));

        //   i = params - 1
        //    for j in range(params - 3) :
        //       print(i, j)
        //       X.select(1, i).copy_(dy.narrow(0, j, neq))
        //       i -= 1

        auto adfs_count = maxw - minw; // number of adfs for one sadf t
        auto nadf = maxw; // data used is (maximum - first adf)
        auto neq_adf = maxw - order - 1; // number of equation for longest adf (maxw)
        // 1 sadf point requires at least GB (only matrix storage)
        auto xsize = (neq_adf * params) * 4; // 4 bytes float32
        auto sadft_GB = float(xsize * adfs_count / GIGABytes); // matrix storage for 1 sadf point
        auto nsadft = n - maxw + 1; // number of sadf t's to calculate the entire SADF

        auto batch_size = 1; // number of sadft points to calculate at once
        if (sadft_GB < gpumem_gb) { // each batch will have at least 1GB in OLS matrixes
            batch_size = int(gpumem_gb / sadft_GB);
            batch_size = (batch_size > nsadft) ? nsadft : batch_size; // in case bigger than sadf
        }

        auto nbatchs = nsadft / batch_size; // number of batchs to sent to GPU
        auto lst_batch_size = nsadft - nbatchs * batch_size; //last batch of adfs(integer%)

        if (verbose) {
            std::cout << "maximum ADF window: " << maxw << std::endl;
            std::cout << "minimum ADF window: " << minw << std::endl;
            std::cout << "ADFs per SADF(t) point: " << adfs_count << std::endl;
            std::cout << "one SADF(t) point - all ADF OLS systems storage (GB): " << sadft_GB << std::endl;
            std::cout << "batch size (number of SADF(t) points): " << batch_size << std::endl;
            std::cout << "total number of SADF(t) points: " << nsadft << std::endl;
            std::cout << "number of batchs : " << nbatchs << std::endl;
            std::cout << "last batch size(number of SADF(t) points): " << lst_batch_size << std::endl;
            std::cout << "parameters : " << params << std::endl;
            std::cout << "number of equations master ADF : " << neq << std::endl;
            std::cout << "number of equations max ADF : " << neq_adf << std::endl;
            std::cout << "total number of ADF tests per batch (solved at once): " << batch_size * adfs_count << std::endl;
        }

        // master X for a sadft (biggest adf OLS X matrix)
        auto Xm = th::zeros({ batch_size, neq_adf, params }, dtype_option.device(deviceCPU));
        // master Z for a sadft (biggest adf OLS independent term)
        auto zm = th::zeros({ batch_size, neq_adf }, dtype_option.device(deviceCPU));

        // pin memory to faster CPU to GPU  copy
        // trying to make GPU less idle so far not tested again
        //Xbt = Xbt.pin_memory();
        //zbt = zbt.pin_memory();
        //nobt = nobt.pin_memory();

        // CUDA (if existent) - same as above to batch operation
        auto Xcu = th::zeros({ batch_size * adfs_count, neq_adf, params }, dtype_option.device(deviceifGPU));
        auto zcu = th::zeros({ batch_size * adfs_count, neq_adf, 1 }, dtype_option.device(deviceifGPU));
        // result
        auto sadf = th::zeros({ nsadft }, dtype_option.device(deviceifGPU));
        auto adfdispersion = th::zeros({ nsadft }, dtypeL_option.device(deviceifGPU));
        auto eye = th::eye(params, dtype_option.device(deviceifGPU));
        // statistical degrees of freedom for the t statistic test is allways the
        // number of equations - number of params (decrease as the length of data reduces)
        // the same based on the adf sequence inside a SADF(t) calculation
        //auto nobsc = (maxw - p - 1 - (p + 3) - th::arange(maxw - minw));
        auto dgfree = neq_adf - params - th::arange(maxw - minw);  //decreasing as we walk to minw
        auto dgfreecu = dgfree.repeat(batch_size).to(deviceifGPU);

        auto tline = 0; // start line for master main X OLS matrix/ z vector
        for (int i = 0; i < nbatchs; i++) {
            //# copy the batch_size first SADF(t) points ADF bigger matrices
            // for j in range(batch_size) :
            //        Xmb.select(0, j).copy_(X.narrow(0, t, nobsadf))
            //        zmb.select(0, j).copy_(z.narrow(0, t, nobsadf))
            //        t = t + 1
            for (int j = 0; j < batch_size; j++) { // assembly batch_size sadf'ts matrixes
                Xm.select(0, j).copy_(X.narrow(0, tline, neq_adf)); //  master X for this sadft - biggest adf OLS X matrix
                zm.select(0, j).copy_(z.narrow(0, tline, neq_adf)); // master Z for this sadft (biggest adf OLS independent term)
                tline++;
            }
            //# repeat those matrix for the number of ADF's of each SADF(t) point
            //Xcpu = Xmb.repeat([1, adfs_count, 1]).view(batch_size, adfs_count, nobsadf, p + 3)
            //zcpu = zmb.repeat([1, adfs_count]).view(batch_size, -1, nobsadf)
            auto Xcpu = Xm.repeat({ 1, adfs_count, 1 }).view({ batch_size, adfs_count, neq_adf, params });
            auto zcpu = zm.repeat({ 1, adfs_count }).view({ batch_size, -1, neq_adf });
            //for k in range(adfs_count) : # each is smaller than previous
            //    # Xbt[j, k, :k, : ] = 0
            //    # zbt[j, k, :k] = 0
            //    Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            //    zcpu.select(1, k).narrow(1, 0, k).fill_(0)
            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
                Xcpu.select(1, k).narrow(1, 0, k).fill_(0);
                zcpu.select(1, k).narrow(1, 0, k).fill_(0);
            }
            //Xcu.copy_(Xcpu.view(batch_size* adfs_count, nobsadf, (3 + p)))
            //zcu.copy_(zcpu.view(batch_size* adfs_count, nobsadf, 1))
            Xcu.copy_(Xcpu.view({ batch_size * adfs_count, neq_adf, params }), true);
            zcu.copy_(zcpu.view({ batch_size * adfs_count, neq_adf, 1 }), true);

            auto XcuT = Xcu.transpose(1, -1);
            auto L = Cholesky(XcuT.bmm(Xcu));
            auto xtz = XcuT.bmm(zcu);
            // auto fileout = std::ofstream("bug_cholesky_solve_L.txt", std::ofstream::out);
            // PrintMatrices(L, fileout);
            // fileout.close();
            // fileout = std::ofstream("bug_cholesky_solve_v.txt", std::ofstream::out);
            // PrintMatrices(xtz, fileout);
            // fileout.close();
            auto Bhat = th::cholesky_solve(xtz, L);
            auto Gi = th::cholesky_solve(eye, L); // (X ^ T.X) ^ -1
            //auto Bhat = std::get<0>(th::triangular_solve(XcuT.bmm(zcu), L, false));
            //auto Gi = std::get<0>(th::triangular_solve(eye, L, false)); // (X ^ T.X) ^ -1
            auto er = zcu - Xcu.bmm(Bhat);
            Bhat = Bhat.squeeze();
            auto s2 = (er * er).sum(1).squeeze().div(dgfreecu);
            // tstats = Bhat[:, 2] / th.sqrt(s2 * Gi[:, 2, 2]) - for params = 3 // bellow generic
            auto adfstats = Bhat.select(-1, 2 - !drift).div(th::sqrt(Gi.select(-2, 2 - !drift).select(-1, 2 - !drift) * s2));
            // in case colinearity causes singular matrices, set -FLT_MAX for those ADF values
            // so in comparison it is allways the smallest value
            adfstats.index_fill_(0, th::nonzero(th::isnan(adfstats)).view(-1), ADF_ERROR);

            // Quantile ADF - more robust than simple max
            // percentile with percentile dispersion -  page 256 reference book -  Quantile ADF
            auto adfstats_sorted = std::get<0>(th::sort(adfstats.view({ batch_size, adfs_count })));
            // adfstats_sorted.select(-1, BATCH_PERC_IDX(0.90, adfs_count)) // SADF value ~ max of ADFs at Percentil 0.90
            // quotting book "...as a measure of centrality of high ADF values..." Q(SADFt, 0.90)
            sadf.narrow(0, tline - batch_size, batch_size).copy_(adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC, adfs_count)));
            // quotting book "...as a measure of dispersion of high ADF values..."  Q(SADFt,0.90, 0.5)=Q(SADFt,0.95)-Q(SADFt,0.85)
            adfdispersion.narrow(0, tline - batch_size, batch_size).copy_(
                adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC+QSADF_SPREAD, adfs_count)) -
                adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC-QSADF_SPREAD, adfs_count)));

            // SIMPLE SADF with adfmaxidx:  index of maximum ADF value
            //auto max = adfstats.view({ batch_size, adfs_count }).max(-1);
            //sadf.narrow(0, tline - batch_size, batch_size).copy_(std::get<0>(max));
            //adfmaxidx.narrow(0, tline - batch_size, batch_size).copy_(std::get<1>(max));
        }

        if (lst_batch_size > 0) {

            dgfreecu = dgfree.repeat(lst_batch_size).to(deviceifGPU);
            Xm = Xm.narrow(0, 0, lst_batch_size);
            zm = zm.narrow(0, 0, lst_batch_size);

            for (int j = 0; j < lst_batch_size; j++) { // assembly batch_size sadf'ts matrixes
                Xm.select(0, j).copy_(X.narrow(0, tline, neq_adf)); //  master X for this sadft - biggest adf OLS X matrix
                zm.select(0, j).copy_(z.narrow(0, tline, neq_adf)); // master Z for this sadft (biggest adf OLS independent term)
                tline++;
            }
            //# repeat those matrix for the number of ADF's of each SADF(t) point
            //Xcpu = Xmb.repeat([1, adfs_count, 1]).view(batch_size, adfs_count, nobsadf, p + 3)
            //zcpu = zmb.repeat([1, adfs_count]).view(batch_size, -1, nobsadf)
            auto Xcpu = Xm.repeat({ 1, adfs_count, 1 }).view({ lst_batch_size, adfs_count, neq_adf, params });
            auto zcpu = zm.repeat({ 1, adfs_count }).view({ lst_batch_size, -1, neq_adf });
            //for k in range(adfs_count) : # each is smaller than previous
            //    # Xbt[j, k, :k, : ] = 0
            //    # zbt[j, k, :k] = 0
            //    Xcpu.select(1, k).narrow(1, 0, k).fill_(0)
            //    zcpu.select(1, k).narrow(1, 0, k).fill_(0)
            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
                Xcpu.select(1, k).narrow(1, 0, k).fill_(0);
                zcpu.select(1, k).narrow(1, 0, k).fill_(0);
            }
            //Xcu.copy_(Xcpu.view(batch_size* adfs_count, nobsadf, (3 + p)))
            //zcu.copy_(zcpu.view(batch_size* adfs_count, nobsadf, 1))
            Xcu = Xcu.narrow(0, 0, lst_batch_size * adfs_count);
            zcu = zcu.narrow(0, 0, lst_batch_size * adfs_count);
            Xcu.copy_(Xcpu.view({ lst_batch_size * adfs_count, neq_adf, params }), true);
            zcu.copy_(zcpu.view({ lst_batch_size * adfs_count, neq_adf, 1 }), true);

            auto XcuT = Xcu.transpose(1, -1);
            auto L = Cholesky(XcuT.bmm(Xcu));
            auto xtz = XcuT.bmm(zcu);
            auto Bhat = th::cholesky_solve(xtz, L, false);
            auto Gi = th::cholesky_solve(eye, L, false); // (X ^ T.X) ^ -1
            //auto Bhat = std::get<0>(th::triangular_solve(XcuT.bmm(zcu), L, false));
            //auto Gi = std::get<0>(th::triangular_solve(eye, L, false)); // (X ^ T.X) ^ -1
            auto er = zcu - Xcu.bmm(Bhat);
            Bhat = Bhat.squeeze();
            auto s2 = (er * er).sum(1).squeeze().div(dgfreecu);
            // tstats = Bhat[:, 2] / th.sqrt(s2 * Gi[:, 2, 2]) - for params = 3 // bellow generic
            auto adfstats = Bhat.select(-1, 2 - !drift).div(th::sqrt(Gi.select(-2, 2 - !drift).select(-1, 2 - !drift) * s2));
            // in case colinearity causes singular matrices, set -FLT_MAX for those ADF values
            // so in comparison it is allways the smallest value
            adfstats.index_fill_(0, th::nonzero(th::isnan(adfstats)).view(-1), ADF_ERROR);


            // Quantile ADF
            // percentile with percentile dispersion -  page 256 reference book -  Quantile ADF
            auto adfstats_sorted = std::get<0>(th::sort(adfstats.view({ lst_batch_size, adfs_count })));
            //adfstats_sorted.select(-1, BATCH_PERC_IDX(0.90, adfs_count)) // SADF value ~ max of ADFs at Percentil 0.90
            // quotting book "...as a measure of centrality of high ADF values..." Q(SADFt, 0.90)
            sadf.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC, adfs_count)));
            // quotting book "...as a measure of dispersion of high ADF values..."  Q(SADFt,0.90, 0.5)=Q(SADFt,0.85)-Q(SADFt,0.95)
            adfdispersion.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(
                adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC+QSADF_SPREAD, adfs_count)) -
                adfstats_sorted.select(-1, PERCENTIL_IDX(QSADF_PERC-QSADF_SPREAD, adfs_count)));

            // Legacy SADF
            //auto max = adfstats.view({ lst_batch_size, adfs_count }).max(-1);
            //sadf.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(std::get<0>(max));
            //adfmaxidx.narrow(0, tline - lst_batch_size, lst_batch_size).copy_(std::get<1>(max));
        }


        sadf = sadf.to(deviceCPU);
        adfdispersion = th::_cast_Float(adfdispersion.to(deviceCPU));

        memcpy(outsadf, sadf.data_ptr<float>(), sizeof(float) * nsadft);
        memcpy(outadfmaxdispersion, adfdispersion.data_ptr<float>(), sizeof(float) * nsadft);

        return nsadft;

#ifdef  FILEDEBUG
        }
        catch (const std::exception& ex) {
            debugfile << "c++ exception: " << std::endl;
            debugfile << ex.what() << std::endl;
        }
        catch (...) {
            debugfile << "Weird no idea exception" << std::endl;
            //cmpbegin_time = -1;<
        }
#endif //  DEBUG

    // working perfectly - only one GPU so only one thread can access it a time
    // debugfile << "sadfd_mt5 unlocking thread" << std::endl;

        return -1;
}


int sadfd_mt5(double* signal, double* outsadf, double* lagout, int n, int maxw, int minw, int order, bool drift, double gpumem_gb, bool verbose) {
    float* fsignal = new float[n];
    float* foutsadf = new float[n - maxw];
    float* flagout = new float[n - maxw];

    float last_valid = 0;
    int count_evalues = 0; // count of invalid values filled

    try {
        // convert from double to float
        for (int i = 0; i < n; i++)
            fsignal[i] = (float)signal[i];

        auto ns = sadf(fsignal, foutsadf, flagout, n, maxw, minw, order, drift, (float)gpumem_gb, verbose);

        for (int i = 0; i < n - maxw; i++) {
            outsadf[i] = (double) foutsadf[i];
            lagout[i] = (double) flagout[i];
        }

        delete[] fsignal;
        delete[] foutsadf;
        delete[] flagout;
    }
    catch (const std::exception& ex) {
        debugfile << "c++ exception: " << std::endl;
        debugfile << ex.what() << std::endl;
    }
    catch (...) {
        debugfile << "Weird no idea exception" << std::endl;
    }

    return count_evalues;
}



////Using Dataset to overcome low GPU usage: GPU spikes with big space (delay) in between
//
////Supremum Augmented dickey fuller test OLS matrices as a Dataset
////One sample is composed of all OLS's data needed to perform the ADF's to calculate one sadf(t) point
//class SADFtMatrices : public torch::data::Dataset<SADFtMatrices>
//{
//    private:
//        bool verbose;
//        int maxw;
//        int minw;
//        int p; // order of AR Model
//        th::Tensor X; // Mather X OLS matrix, all OLS adf matrices are inside this
//        th::Tensor z;
//        int adfs_count; // number of adfs for one sadf t
//        int nobsadf; //number of observations(maximum - first adf)
//        int nsadft; // number of sadf t's to calculate the entire SADF
//
//    public:
//        explicit SADFtMatrices(float *signal, int n, int maxw, int minw, int p = 30, bool verbose = false){
//            maxw = maxw;
//            minw = minw;
//            p = p;
//            verbose = verbose;
//            th::NoGradGuard guard; // same as with torch.no_grad(): block
//
//            auto nobs = n - p - 1; // data used for regression
//            X = th::zeros({ nobs, 3 + p }, dtype_option);
//            auto y = th::from_blob(signal, { n }, dtype_option);
//
//            auto diffilter = th::tensor({ -1, 1 }, dtype_option).view({ 1, 1, 2 }); // first difference filter
//            auto dy = th::conv1d(y.view({ 1, 1, -1 }), diffilter).view({ -1 });
//            z = dy.slice(0, p).clone();
//
//            // fill in first 3 columns, drit, trend, reg. data
//            // acessors or tensor[i][j].item<int>()
//            auto ay = y.accessor<float, 1>();
//            auto aX = X.accessor<float, 2>(); // drift
//            auto ady = dy.accessor<float, 1>();
//            for (auto i = 0; i < nobs; i++) {
//                aX[i][0] = 1; // drift term
//                aX[i][1] = p + 1 + i; // deterministic trend
//                aX[i][2] = ay[p + i];  // regression data(nobs)
//            }
//            // fill in other columns, start at third
//            for (auto j = 0; j < nobs; j++)
//                for (auto i = 1; i < p + 1; i++) //X[:, 2+i] = dy[p-i:-i]
//                    aX[j][2 + i] = ady[p - i + j];
//
//            adfs_count = maxw - minw;
//            auto nadf = maxw; //  data used is (maximum - first adf)
//            nobsadf = nadf - p - 1;
//            nsadft = n - maxw;
//            p = p;
//
//            if (verbose) {
//                std::cout << "ADF tests calculated for one sadf(t) " << adfs_count << std::endl;
//                std::cout << "Number of sadf(t) points " <<  nsadft << std::endl;
//            }
//
//        };
//
//        // Override the get method to load custom data.
//        // index :
//        // index of a specific sadf(t) point calculation
//        // same as start line for master main X OLS matrix / z vector
//        // returns all statistic tests elements of all ADF's to calculate this sadf(t) point
//        // tuple(OLS X, OLS z, nobservations per ADF)
//        torch::data::Example<> get(size_t index) override {
//            th::NoGradGuard guard; // same as with torch.no_grad(): block
//
//            auto nobts = th::zeros({ adfs_count }, dtype_option.device(deviceCPU));
//
//            // Xm // master X for this sadft - biggest adf OLS X matrix
//            // zm // master Z for this sadft(biggest adf OLS independent term)
//            auto Xm = X.narrow(0, index, nobsadf); //  master X for this sadft - biggest adf OLS X matrix
//            auto zm = z.narrow(0, index, nobsadf); // master Z for this sadft (biggest adf OLS independent term
//            auto Xbts = Xm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf, (3 + p) });
//            auto zbts = zm.repeat({ adfs_count, 1 }).view({ adfs_count, nobsadf });
//
//            for (int k = 0; k < adfs_count; k++) { //each is smaller than previous
//                //    sadf loop until minw, every matrix is smaller than the previous
//                //    zeroing i lines, observations are becomming less also
//                Xbts.select(0, k).narrow(0, 0, k).fill_(0);
//                zbts.select(0, k).narrow(0, 0, k).fill_(0);
//                nobts.select(0, k).fill_(float(nobsadf - k - (p + 3)));
//            }
//
//            // due problem design with this class not accepting 3 objects as a return
//            // have to create fake target adding 1 item on zm vector
//            // tha corresponds to the number of observations of the ADF test
//            auto fake_target = th::zeros({ adfs_count, nobsadf + 1 });
//            fake_target.narrow(1, 0, nobsadf).copy_(zbts);
//            fake_target.narrow(1, nobsadf, 1).view(-1).copy_(nobts);
//
//            // is not needed according to
//            // https://discuss.pytorch.org/t/how-to-manually-delete-free-a-tensor-in-aten/64153/4?u=eusouoandre
//            //delete []&nobts;
//            //delete []&zbts;
//
//            return {Xbts, fake_target};
//        };
//
//        // Override the size method to infer the size of the data set.
//        torch::optional<size_t> size() const override {
//            return nsadft;
//        };
//};
