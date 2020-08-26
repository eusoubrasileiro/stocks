#pragma once
#include "libpytorch.h"

th::Tensor thfdcoefs;
// calculate the fractdif coeficients to be used next
// $$ \omega_{k} = -\omega_{k-1}\frac{d-k+1}{k} $$
// output is allocated inside to a pytorch tensor
// on current device
void setfracdiffcoefs(float d, int size) {
    th::NoGradGuard guard; // same as with torch.no_grad(): block
    auto w = new float[size];
    w[0] = 1.;
    for (int k = 1; k < size; k++)
        w[k] = -w[k - 1] / k * (d - k + 1);
    std::reverse(w, w + size);
    thfdcoefs = th::from_blob(w, { 1, 1, size }, dtype32_option);
    // to GPU or not
    thfdcoefs.to(deviceifGPU);
}

// apply fracdif filter on signal array
// FracDifCoefs must be supplied
// output is allocated inside
int fracdiffapply(float signal[], int size, float output[]) {
    th::NoGradGuard guard; // same as with torch.no_grad(): block
    th::Tensor thdata = th::from_blob(signal, { 1, 1, size }, dtype32_option).clone();
    // to GPU or not
    thdata.to(deviceifGPU);
    th::Tensor thresult = th::conv1d(thdata, thfdcoefs).reshape({ -1 });
    // back to CPU
    thresult = thresult.to(deviceCPU);
    // double array output
    auto ptr_data = thresult.data_ptr<float>();
    int outsize = thresult.size(0);
    memcpy(output, ptr_data, sizeof(float) * outsize);

    return outsize;
}