// Copyright 2018 The AITS DNNC Authors.All Rights Reserved.
//
// Licensed to the Apache Software Foundation(ASF) under one
// or more contributor license agreements.See the NOTICE file
// distributed with this work for additional information
// regarding copyright ownership.The ASF licenses this file
// to you under the Apache License, Version 2.0 (the
// "License"); you may not use this file except in compliance
// with the License.You may obtain a copy of the License at
//
//   http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing,
// software distributed under the License is distributed on an
// "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
// KIND, either express or implied.See the License for the
// specific language governing permissionsand limitations
// under the License.
//
// This file is part of AITS DNN compiler maintained at
// https://github.com/ai-techsystems/dnnCompiler
//

#pragma once
#include "operators/baseOperator.h"
#include <string>

using namespace Eigen;

namespace dnnc {
template <typename T> class Slice : public baseOperator<T, T, T> {
  //  Slice attributes
public:
  Slice(std::string name = "opSlice") : baseOperator<T, T, T>(opSlice, name) {}

  // bool getAttribute<int>(OPATTR attrName, int& obj) ;

  tensor<T> compute(
      tensor<T> a,             // N-D Tensor of data to extract slices from
      tensor<DIMENSION> start, // 1-D tensor of starting indices of
                               // corresponding axis in `axes`
      tensor<DIMENSION> end,   // 1-D tensor of ending indices (exclusive) of
                               // corresponding axis in `axes`
      tensor<int> axes = NULL_TENSOR<
          int>, // 1-D tensor of axes that `starts` and `ends` apply to.
                // Negative value means counting dimensions from the back.
      tensor<DIMENSION> steps =
          NULL_TENSOR<DIMENSION>) // 1-D tensor of slice step of corresponding
                                  // axis in `axes`. Default to 1.
  {
    // Process and check the arguments

    std::stringstream errMsg;

    DIMENSION num_axes = start.shape()[0];
    int rank = a.rank();

    if (start.rank() != 1) {
      errMsg << "start tensor is " << start.rank()
             << "dimensional (should be 1 dimensional)" << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (end.rank() != 1) {
      errMsg << "end tensor is " << end.rank()
             << "dimensional (should be 1 dimensional)" << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (start.shape() != end.shape()) {
      errMsg << "start and end tensor sizes don't match (";
      errMsg << "start tensor size = " << start.shape()[0] << ", ";
      errMsg << "end tensor size = " << end.shape()[0] << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (axes == NULL_TENSOR<int>) {
      std::vector<DIMENSION> shape{num_axes};
      tensor<int> default_axis(shape);
      axes = default_axis;
      for (size_t i = 0; i < num_axes; i++) {
        axes(i) = i + 1;
      }
    }

    if (steps == NULL_TENSOR<DIMENSION>) {
      std::vector<DIMENSION> shape{num_axes};
      tensor<DIMENSION> default_steps(shape);
      steps = default_steps;
      for (size_t i = 0; i < num_axes; i++) {
        steps(i) = 1;
      }
    }

    if (axes.rank() != 1) {
      errMsg << "axes tensor is " << axes.rank()
             << "dimensional (should be 1 dimensional)" << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (steps.rank() != 1) {
      errMsg << "steps tensor is " << steps.rank()
             << "dimensional (should be 1 dimensional)" << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (start.shape() != axes.shape()) {
      errMsg << "start and axes tensor sizes don't match (";
      errMsg << "start tensor size = " << start.shape()[0] << ", ";
      errMsg << "axes tensor size = " << axes.shape()[0] << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    if (start.shape() != steps.shape()) {
      errMsg << "start and axes tensor sizes don't match (";
      errMsg << "start tensor size = " << start.shape()[0] << ", ";
      errMsg << "steps tensor size = " << steps.shape()[0] << std::endl;
      throw std::invalid_argument(errMsg.str().c_str());
    }

    for (size_t i = 0; i < num_axes; i++) {

      // start
      if (start(i) >= a.shape()[i]) {
        errMsg << "start value (" << start(i) << ") along axis " << i
               << " is beyond the size (" << a.shape()[i]
               << ") of input tensor along the axis" << std::endl;
        throw std::invalid_argument(errMsg.str().c_str());
      }

      // end
      if (end(i) > (a.shape()[i] - 1)) {
        errMsg << "end value (" << end(i) << ") along axis " << i
               << " is beyond the size (" << a.shape()[i]
               << ") of input tensor along the axis" << std::endl;
        throw std::invalid_argument(errMsg.str().c_str());
      }
      if ((end(i) - 1) < start(i)) {
        errMsg << "end value (" << end(i) - 1 << ") along axis" << i
               << " is smaller than the start value (" << start(i)
               << ") along the axis" << std::endl;
        throw std::invalid_argument(errMsg.str().c_str());
      }

      // axes
      if (axes(i) < 0) {
        if ((axes(i) + rank) < 0) {
          errMsg << "axes value (" << axes(i) << ") along axis " << i
                 << " is beyond the input tensor dimension" << std::endl;
          throw std::invalid_argument(errMsg.str().c_str());
        }
        axes(i) = rank + axes(i);
      }
      if (axes(i) > rank) {
        errMsg << "axes value (" << axes(i) << ") along axis " << i
               << " is large than the number of dimensions of input tensor"
               << std::endl;
        throw std::invalid_argument(errMsg.str().c_str());
      }

      for (size_t j = i + 1; j < num_axes; j++) {
        if (axes(i) == axes(j)) {
          errMsg << "repeated axis value (" << axes(i) << ") at indices " << i
                 << " and " << j << " of axes input" << std::endl;
          throw std::invalid_argument(errMsg.str().c_str());
        }
      }

      // steps
    }

    // Determine the shape of the result tensor

    std::vector<size_t> resultShape(rank);
    std::vector<size_t> start_index(rank);
    std::vector<size_t> end_index(rank);
    std::vector<size_t> step(rank);

    for (int axis = 0; axis < rank; axis++) {

      // determine slicing along the axis-th dimension
      for (size_t i = 0; i < num_axes; i++) {
        if ((axes(i) - 1) == axis) {
          start_index[axis] = start(i);
          end_index[axis] = end(i) - 1;
          step[axis] = steps[i];
          break;
        } else {
          start_index[axis] = 0;
          end_index[axis] = a.shape()[axis] - 1;
          step[axis] = 1;
        }
      }

      resultShape[axis] =
          (end_index[axis] - start_index[axis]) / step[axis] + 1;
    }

    // Slice now
    tensor<T> result(resultShape);

    if (rank == 1) {
      size_t i0 = 0;
      for (size_t _i0 = start_index[0]; _i0 <= end_index[0]; _i0 += step[0]) {
        result(i0++) = a(_i0);
      }
    } else if (rank == 2) {
      size_t i0 = 0;
      for (size_t _i0 = start_index[0]; _i0 <= end_index[0]; _i0 += step[0]) {
        size_t i1 = 0;
        for (size_t _i1 = start_index[1]; _i1 <= end_index[1]; _i1 += step[1]) {
          result(i0, i1++) = a(_i0, _i1);
        }
        i0++;
      }
    } else if (rank == 3) {
      size_t i0 = 0;
      for (size_t _i0 = start_index[0]; _i0 <= end_index[0]; _i0 += step[0]) {
        size_t i1 = 0;
        for (size_t _i1 = start_index[1]; _i1 <= end_index[1]; _i1 += step[1]) {
          size_t i2 = 0;
          for (size_t _i2 = start_index[2]; _i2 <= end_index[2];
               _i2 += step[2]) {
            result(i0, i1, i2++) = a(_i0, _i1, _i2);
          }
          i1++;
        }
        i0++;
      }
    } else if (rank == 4) {
      size_t i0 = 0;
      for (size_t _i0 = start_index[0]; _i0 <= end_index[0]; _i0 += step[0]) {
        size_t i1 = 0;
        for (size_t _i1 = start_index[1]; _i1 <= end_index[1]; _i1 += step[1]) {
          size_t i2 = 0;
          for (size_t _i2 = start_index[2]; _i2 <= end_index[2];
               _i2 += step[2]) {
            size_t i3 = 0;
            for (size_t _i3 = start_index[3]; _i3 <= end_index[3];
                 _i3 += step[3]) {
              result(i0, i1, i2, i3++) = a(_i0, _i1, _i2, _i3);
            }
            i2++;
          }
          i1++;
        }
        i0++;
      }
    } else {
      std::cout << "Not supported";
      return NULL_TENSOR<T>;
    }

    return result;
  }
};
} // namespace dnnc
