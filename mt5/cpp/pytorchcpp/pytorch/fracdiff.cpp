#pragma once
#include "libpytorch.h"

th::Tensor thfdcoefs;
// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setfracdiffcoefs(double d, int size) {
    th::NoGradGuard guard; // same as with torch.no_grad(): block
    double* w = new double[size];
    w[0] = 1.;
    for (int k = 1; k < size; k++)
        w[k] = -w[k - 1] / k * (d - k + 1);
    std::reverse(w, w + size);
    thfdcoefs = th::from_blob(w, { 1, 1, size }, dtype64_option);
    // to GPU or not
    thfdcoefs.to(deviceifGPU);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int fracdiffapply(double signal[], int size, double output[]) {
    th::NoGradGuard guard; // same as with torch.no_grad(): block
    th::Tensor thdata = th::from_blob(signal, { 1, 1, size }, dtype64_option).clone();
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