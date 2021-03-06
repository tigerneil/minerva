#pragma once
#include <curand_kernel.h>

// Unary functions
class ExpOp {
 public:
  __device__ inline float operator()(float a) const {
    return expf(a);
  }
};

class LnOp {
 public:
  __device__ inline float operator()(float a) const {
    return logf(a);
  }
};

class NegativeOp {
 public:
  __device__ inline float operator()(float a) const {
    return -a;
  }
};

// Binary function
class SubOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return a - b;
  }
};

class ReverseSubOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return b - a;
  }
};

class MultOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return a * b;
  }
};

class DivOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return a / b;
  }
};

class ReverseDivOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return b / a;
  }
};

class MaxOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return b < a ? a : b;
  }
};

class SumOp {
 public:
  __device__ inline float operator()(float a, float b) const {
    return a + b;
  }
};

// c = a DotOp b
template<typename Func>
__global__ static void CudaPerformDotKernel(float* a, float* b, float* c, size_t size, Func func) {
  int cur = threadIdx.x + blockIdx.x * blockDim.x;
  while (cur < size) {
    *(c + cur) = func(*(a + cur), *(b + cur));
    cur += gridDim.x * blockDim.x;
  }
}

// y = x Op v
template<typename Func>
__global__ static void CudaPerformDotKernel(float* x, float* y, float v, size_t size, Func func) {
  int cur = threadIdx.x + blockIdx.x * blockDim.x;
  while (cur < size) {
    y[cur] = func(x[cur], v);
    cur += gridDim.x * blockDim.x;
  }
}

// y = Op (x)
template<typename Func>
__global__ static void CudaPerformDotKernel(float* x, float* y, size_t size, Func func) {
  int cur = threadIdx.x + blockIdx.x * blockDim.x;
  while (cur < size) {
    y[cur] = func(x[cur]);
    cur += gridDim.x * blockDim.x;
  }
}

// res = matrix Norm row
template<typename Func>
__global__ static void CudaPerformNormOnColKernel(float* matrix, float* row, float * res, int m, int n, Func func) {
  int row_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (row_id < m) {
    for (int i = 0; i < n; ++i) {
      res[i*m + row_id] = func(matrix[i*m + row_id], row[i]);
    }
    row_id += step;
  }
}

// res = matrix Norm col
template<typename Func>
__global__ static void CudaPerformNormOnRowKernel(float* matrix, float* col, float * res, int m, int n, Func func) {
  int row_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (row_id < m) {
    for (int i = 0; i < n; ++i) {
      res[i*m + row_id] = func(matrix[i*m + row_id], col[row_id]);
    }
    row_id += step;
  }
}

// row = ReductionOp(matrix)
template<typename Func>
__global__ static void CudaPerformReductionOnColKernel(float* matrix, float* row, int m, int n, Func func) {
  // TODO: this is inefficient
  int col_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (col_id < n) {
    float r = matrix[col_id * m];
    for (int i = 1; i < m; ++i) {
      r = func(r, matrix[col_id * m + i]);
    }
    row[col_id] = r;
    col_id += step;
  }
}

// col = ReductionOp(matrix)
template<typename Func>
__global__ static void CudaPerformReductionOnRowKernel(float* matrix, float* col, int m, int n, Func func) {
  int row_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (row_id < m) {
    float r = matrix[row_id];
    for (int i = 1; i < n; ++i) {
      r = func(r, matrix[i * m + row_id]);
    }
    col[row_id] = r;
    row_id += step;
  }
}

// row = MaxIndexOp(matrix)
__global__ static void CudaPerformMaxIndexOnColKernel(float* matrix, float* row, int m, int n) {
  // TODO: this is inefficient
  int col_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (col_id < n) {
    float maxv = matrix[col_id * m];
    int maxid = 0;
    for (int i = 1; i < m; ++i) {
      if (maxv < matrix[col_id * m + i]) {
        maxv = matrix[col_id * m + i];
        maxid = i;
      }
    }
    row[col_id] = maxid;
    col_id += step;
  }
}

// col = MaxIndexOp(matrix)
__global__ static void CudaPerformMaxIndexOnRowKernel(float* matrix, float* col, int m, int n) {
  int row_id = threadIdx.x + blockIdx.x * blockDim.x;
  int step = gridDim.x * blockDim.x;
  while (row_id < m) {
    float maxv = matrix[row_id];
    int maxid = 0;
    for (int i = 1; i < n; ++i) {
      if (maxv < matrix[i * m + row_id]) {
        maxv = matrix[i * m + row_id];
        maxid = i;
      }
    }
    col[row_id] = maxid;
    row_id += step;
  }
}

__global__ static void CudaPerformRandBernoulliKernel(float* dst, size_t size, unsigned int seed, float p) {
  int step = gridDim.x * blockDim.x;
  int cur = threadIdx.x + blockIdx.x * blockDim.x;
  curandState_t state;
  curand_init(seed, cur, 0, &state);
  while (cur < size) {
    if (curand_uniform(&state) < p) {
      dst[cur] = 1;
    } else {
      dst[cur] = 0;
    }
    cur += step;
  }
}

__global__ static void CudaPerformFillKernel(float* dst, size_t size, float val) {
  int cur = threadIdx.x + blockIdx.x * blockDim.x;
  while (cur < size) {
    dst[cur] = val;
    cur += gridDim.x * blockDim.x;
  }
}

__global__ static void CudaMaxPoolForward(const int nthreads, const float* bottom_data,
    const int num, const int channels, const int height,
    const int width, const int pooled_height, const int pooled_width,
    const int kernel_h, const int kernel_w, const int stride_h,
    const int stride_w, const int pad_h, const int pad_w, float* top_data) {
	for (int index = blockIdx.x * blockDim.x + threadIdx.x; index < nthreads; index += blockDim.x * gridDim.x )
	{
		int pw = index % pooled_width;
		int ph = (index / pooled_width) % pooled_height;
		int c = (index / pooled_width / pooled_height) % channels;
		int n = index / pooled_width / pooled_height / channels;
		int hstart = ph * stride_h - pad_h;
		int wstart = pw * stride_w - pad_w;
		int hend = min(hstart + kernel_h, height);
		int wend = min(wstart + kernel_w, width);
		hstart = max(hstart, 0);
		wstart = max(wstart, 0);
		float maxval = -3.4E+38F;
		int maxidx = -1;
		bottom_data += (n * channels + c) * height * width;
		for (int h = hstart; h < hend; ++h) {
		  for (int w = wstart; w < wend; ++w) {
			if (bottom_data[h * width + w] > maxval) {
			  maxidx = h * width + w;
			  maxval = bottom_data[maxidx];
			}
		  }
		}
		top_data[index] = maxval;
  }
}

__global__ void CudaMaxPoolBackward(const int nthreads, const float* bottom_data, const float* top_diff, float* bottom_diff, const int num, const int channels,
    const int height, const int width, const int pooled_height,
    const int pooled_width, const int kernel_h, const int kernel_w,
    const int stride_h, const int stride_w, const int pad_h, const int pad_w) {
 	for (int index = blockIdx.x * blockDim.x + threadIdx.x; index < nthreads; index += blockDim.x * gridDim.x )
	{
		int pw = index % pooled_width;
		int ph = (index / pooled_width) % pooled_height;
		int c = (index / pooled_width / pooled_height) % channels;
		int n = index / pooled_width / pooled_height / channels;
		int hstart = ph * stride_h - pad_h;
		int wstart = pw * stride_w - pad_w;
		int hend = min(hstart + kernel_h, height);
		int wend = min(wstart + kernel_w, width);
		hstart = max(hstart, 0);
		wstart = max(wstart, 0);
		float maxval = -3.4E+38F;
		int maxidx = -1;
		bottom_data += (n * channels + c) * height * width;
		bottom_diff += (n * channels + c) * height * width;
		for (int h = hstart; h < hend; ++h) {
		  for (int w = wstart; w < wend; ++w) {
			if (bottom_data[h * width + w] > maxval) {
			  maxidx = h * width + w;
			  maxval = bottom_data[maxidx];
			}
		  }
		}
		bottom_diff[maxidx] += top_diff[index];
  }
}



__global__ static void LRNFillScale(const int nthreads, const float* in, const int num, const int channels, const int height, const int width, const int size, const float alpha_over_size, float* scale) {
	for (int index = blockIdx.x * blockDim.x + threadIdx.x; index < nthreads; index += blockDim.x * gridDim.x )
	{
    // find out the local offset
    int w = index % width;
    int h = (index / width) % height;
    int n = index / width / height;
    int offset = (n * channels * height + h) * width + w;
    int step = height * width;
    in += offset;
    scale += offset;
    int head = 0;
    int pre_pad = (size - 1) / 2;
    int post_pad = size - pre_pad - 1;
    float accum_scale = 0;
    // fill the scale at [n, :, h, w]
    // accumulate values
    while (head < post_pad) {
      accum_scale += in[head * step] * in[head * step];
      ++head;
    }
    // until we reach size, nothing needs to be subtracted
    while (head < size) {
      accum_scale += in[head * step] * in[head * step];
      scale[(head - post_pad) * step] = 1. + accum_scale * alpha_over_size;
      ++head;
    }
    // both add and subtract
    while (head < channels) {
      accum_scale += in[head * step] * in[head * step];
      accum_scale -= in[(head - size) * step] * in[(head - size) * step];
      scale[(head - post_pad) * step] = 1. + accum_scale * alpha_over_size;
      ++head;
    }
    // subtract only
    while (head < channels + post_pad) {
      accum_scale -= in[(head - size) * step] * in[(head - size) * step];
      scale[(head - post_pad) * step] = 1. + accum_scale * alpha_over_size;
      ++head;
    }
  }
}

__global__ static void LRNComputeOutput(const int nthreads, const float* in,
    const float* scale, const float negative_beta, float* out) {
	for (int index = blockIdx.x * blockDim.x + threadIdx.x; index < nthreads; index += blockDim.x * gridDim.x )
	{
		out[index] = in[index] * pow(scale[index], negative_beta);
	}
}

__global__ static void LRNComputeDiff(const int nthreads, const float* bottom_data,
    const float* top_data, const float* scale, const float* top_diff,
    const int num, const int channels, const int height,
    const int width, const int size, const float negative_beta,
    const float cache_ratio,
    float* bottom_diff) {
	
	for (int index = blockIdx.x * blockDim.x + threadIdx.x; index < nthreads; index += blockDim.x * gridDim.x ){
		// find out the local offset
		int w = index % width;
		int h = (index / width) % height;
		int n = index / width / height;
		int offset = (n * channels * height + h) * width + w;
		int step = height * width;
		bottom_data += offset;
		top_data += offset;
		scale += offset;
		top_diff += offset;
		bottom_diff += offset;
		int head = 0;
		int pre_pad = size - (size + 1) / 2;
		int post_pad = size - pre_pad - 1;
		float accum_ratio = 0;
		// accumulate values
		while (head < post_pad) {
		  accum_ratio += top_diff[head * step] * top_data[head * step] /
			  scale[head * step];
		  ++head;
		}
		// until we reach size, nothing needs to be subtracted
		while (head < size) {
		  accum_ratio += top_diff[head * step] * top_data[head * step] /
			  scale[head * step];
		  bottom_diff[(head - post_pad) * step] = top_diff[(head - post_pad) * step]
			  * pow(scale[(head - post_pad) * step], negative_beta) - cache_ratio *
			  bottom_data[(head - post_pad) * step] * accum_ratio;
		  ++head;
		}
		// both add and subtract
		while (head < channels) {
		  accum_ratio += top_diff[head * step] * top_data[head * step] /
			  scale[head * step];
		  accum_ratio -= top_diff[(head - size) * step] *
			  top_data[(head - size) * step] / scale[(head - size) * step];
		  bottom_diff[(head - post_pad) * step] = top_diff[(head - post_pad) * step]
			  * pow(scale[(head - post_pad) * step], negative_beta) - cache_ratio *
			  bottom_data[(head - post_pad) * step] * accum_ratio;
		  ++head;
		}
		// subtract only
		while (head < channels + post_pad) {
		  accum_ratio -= top_diff[(head - size) * step] *
			  top_data[(head - size) * step] / scale[(head - size) * step];
		  bottom_diff[(head - post_pad) * step] = top_diff[(head - post_pad) * step]
			  * pow(scale[(head - post_pad) * step], negative_beta) - cache_ratio *
			  bottom_data[(head - post_pad) * step] * accum_ratio;
		  ++head;
		}
  }
}







