#include "matrix.h"
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <omp.h>

// Include SSE intrinsics
#if defined(_MSC_VER)
#include <intrin.h>
#elif defined(__GNUC__) && (defined(__x86_64__) || defined(__i386__))
#include <immintrin.h>
#include <x86intrin.h>
#endif

/* Below are some intel intrinsics that might be useful
 * void _mm256_storeu_pd (double * mem_addr, __m256d a)
 * __m256d _mm256_set1_pd (double a)
 * __m256d _mm256_set_pd (double e3, double e2, double e1, double e0)
 * __m256d _mm256_loadu_pd (double const * mem_addr)
 * __m256d _mm256_add_pd (__m256d a, __m256d b)
 * __m256d _mm256_sub_pd (__m256d a, __m256d b)
 * __m256d _mm256_fmadd_pd (__m256d a, __m256d b, __m256d c)
 * __m256d _mm256_mul_pd (__m256d a, __m256d b)
 * __m256d _mm256_cmp_pd (__m256d a, __m256d b, const int imm8)
 * __m256d _mm256_and_pd (__m256d a, __m256d b)
 * __m256d _mm256_max_pd (__m256d a, __m256d b)
*/

/* Generates a random double between low and high */
double rand_double(double low, double high) {
    double range = (high - low);
    double div = RAND_MAX / range;
    return low + (rand() / div);
}

/* Generates a random matrix */
void rand_matrix(matrix *result, double low, double high) {
    srand(42);
//    for (int i = 0; i < result->rows; i++) {
//        for (int j = 0; j < result->cols; j++) {
//            set(result, i, j, rand_double(low, high));
//        }
//    }
    int total = result->cols * result->rows;
    #pragma omp parallel for
    for (int i = 0; i < total; ++i) {
        result->data[i] = rand_double(low, high);
    }
}

/*
 * Allocates space for a matrix struct pointed to by the double pointer mat with
 * `rows` rows and `cols` columns. You should also allocate memory for the data array
 * and initialize all entries to be zeros. `parent` should be set to NULL to indicate that
 * this matrix is not a slice. You should also set `ref_cnt` to 1.
 * You should return -1 if either `rows` or `cols` or both have invalid values, or if any
 * call to allocate memory in this function fails. Return 0 upon success.
 */
int allocate_matrix(matrix **mat, int rows, int cols) {
    /* TODO: YOUR CODE HERE */
    // check if row and col are positive
    if (rows <= 0 || cols <= 0) {
        return -1;
    }
    matrix *new_mat = (matrix*) malloc(sizeof(matrix));
    if (new_mat == NULL) {
        return -1;
    }

    int nums = rows * cols;
    double *data = (double*) malloc(sizeof(double) * nums);
    if (data == NULL) {
        free(new_mat); // clean garbage before return failure
        return -1;
    } else {
        for (int i = 0; i < nums; ++i) {
            data[i] = 0.0;
        }
        new_mat->data = data;
        // set entries for new matrix
        new_mat->rows = rows;
        new_mat->cols = cols;
        new_mat->ref_cnt = 1;
        new_mat->parent = NULL;
    }

    *mat = new_mat;
    return 0;
}

/*
 * Allocates space for a matrix struct pointed to by `mat` with `rows` rows and `cols` columns.
 * Its data should point to the `offset`th entry of `from`'s data (you do not need to allocate memory)
 * for the data field. `parent` should be set to `from` to indicate this matrix is a slice of `from`.
 * You should return -1 if either `rows` or `cols` or both are non-positive or if any
 * call to allocate memory in this function fails. Return 0 upon success.
 */
int allocate_matrix_ref(matrix **mat, matrix *from, int offset, int rows, int cols) {
    /* TODO: YOUR CODE HERE */
    if (rows <= 0 || cols <= 0 || from == NULL) {
        return -1;
    }
    matrix *sub_mat = (matrix*) malloc(sizeof(matrix));
    if (sub_mat == NULL) {
        return -1;
    }

    // set entries of sub matrix
    sub_mat->rows = rows;
    sub_mat->cols = cols;
    sub_mat->data = from->data + offset;
    from->ref_cnt += 1; // increment from matrix's ref count
    sub_mat->parent = from;
    sub_mat->ref_cnt = 1;

    *mat = sub_mat;
    return 0;
}

/*
 * This function frees the matrix struct pointed to by `mat`. However, you need to make sure that
 * you only free the data if `mat` is not a slice and has no existing slices, or if `mat` is the
 * last existing slice of its parent matrix and its parent matrix has no other references.
 * You cannot assume that mat is not NULL.
 */
void deallocate_matrix(matrix *mat) {
    /* TODO: YOUR CODE HERE */
    if (mat != NULL) {
        // 1.1 mat is not a slice, and has no existing slices, free matrix and data
        if (mat->parent == NULL && mat->ref_cnt <= 1) {
            free(mat->data);
            free(mat);
        } else if (mat->ref_cnt > 1) { // 1.2 mat has existing slices, decrement ref_cnt by 1
            mat->ref_cnt -= 1;
        } else { // 2. mat is a slice, decrement ref_cnt to data, free mat, if ref_cnt==0, free parent
            mat->parent->ref_cnt -= 1;
            if (mat->parent->ref_cnt == 0) // if parent has no ref, deallocate data
                deallocate_matrix(mat->parent);
            free(mat);
        }
    }
}

/*
 * Returns the double value of the matrix at the given row and column.
 * You may assume `row` and `col` are valid.
 */
double get(matrix *mat, int row, int col) {
    /* TODO: YOUR CODE HERE */
    int offset;
    int cols = mat->cols;
    matrix *mp = mat->parent;
    while (mp != NULL) { // supports nested child
        cols = mp->cols;
        mp = mp->parent;
    }
    offset = row * cols + col;
    return mat->data[offset];
}

/*
 * Sets the value at the given row and column to val. You may assume `row` and
 * `col` are valid
 */
void set(matrix *mat, int row, int col, double val) {
    /* TODO: YOUR CODE HERE */
    int offset;
    int cols = mat->cols;
    matrix *mp = mat->parent;
    while (mp != NULL) { // supports nested child
        cols = mp->cols;
        mp = mp->parent;
    }
    offset = row * cols + col;
    mat->data[offset] = val;
}

/*
 * Sets mat to unit matrix, NOT applicable for a child matrix
 * */
void set_eye(matrix *mat) {
    int total = mat->rows * mat->cols;
    for (int i = 0; i < total; i += 1) {
        if (i / mat->cols == i % mat->cols) {
            mat->data[i] = 1.0;
        } else {
            mat->data[i] = 0.0;
        }
    }
}

/*Copy data of src matrix to dst matrix, returns 0 if ok, 1 otherwise
 * */
int copy_matrix(matrix *dst, matrix *src) {
    if (src->cols == dst->cols && src->rows == dst->rows) {
        int total = dst->rows * dst->cols;
        for (int i = 0; i < total; ++i) {
            dst->data[i] = src->data[i];
        }
        return 0;
    }
    return 1;
}

/*
 * Sets all entries in mat to val
 */
void fill_matrix(matrix *mat, double val) {
    /* TODO: YOUR CODE HERE */
    int total = mat->rows * mat->cols;
    for (int i = 0; i < total; ++i) {
        mat->data[i] = val;
    }
}

/*
 * Store the result of adding mat1 and mat2 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int add_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    /* TODO: YOUR CODE HERE */
    if (result->rows == mat1->rows && result->cols == mat1->cols
        && result->rows == mat2->rows && result->cols == mat2->cols) {
        int total = result->rows * result->cols;
        double *res_mat = result->data;
        double *a_mat = mat1->data;
        double *b_mat = mat2->data;
//        // naive
//        for (int i = 0; i < total; ++i) {
//            res_mat[i] = a_mat[i] + b_mat[i];
//        }
//        return 0;

//        // do unrolling
//        int unroll = 32;
//        for (int i = 0; i < total; i += unroll) {
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//            result->data[i] = mat1->data[i] + mat2->data[i];
//        }
//        // tail case
//        for (int j = total/unroll*unroll; j < total; ++j) {
//            result->data[j] = mat1->data[j] + mat2->data[j];
//        }
//        return 0;

//        // use AVX and unrolling
//        __m256d d1;
//        __m256d d2;
//        __m256d res;
//        int unroll = 1;
//        int stride = 4 * unroll;
//        for (int i = 0; i < total / stride * stride; ) {
//            d1 = _mm256_loadu_pd(mat1->data + i);
//            d2 = _mm256_loadu_pd(mat2->data + i);
//            res = _mm256_add_pd(d1, d2);
//            _mm256_storeu_pd(result->data + i, res);
//            i += 4;
//        }
//        //tail case
//        for (int k = total / stride * stride; k < total / 4 * 4; k += 4) {
//            d1 = _mm256_loadu_pd(mat1->data + k);
//            d2 = _mm256_loadu_pd(mat2->data + k);
//            res = _mm256_add_pd(d1, d2);
//            _mm256_storeu_pd(result->data + k, res);
//        }
//        for (int j = total / 4 * 4; j < total; ++j) {
//            result->data[j] = mat1->data[j] + mat2->data[j];
//        }
//        return 0;

//        //use omp
//        omp_set_num_threads(16);
//        #pragma omp parallel for
//        for (int i = 0; i < total; ++i) {
//            result->data[i] = mat1->data[i] + mat2->data[i];
//        }
//        return 0;

//        //use omp and AVX
//        __m256d d1;
//        __m256d d2;
//        __m256d res;
//        int stride = 4;
//        omp_set_num_threads(16);
//        #pragma omp parallel private(d1, d2, res)
//        {
//            #pragma omp for
//            for (int i = 0; i < total / stride * stride; i += 4) {
//                d1 = _mm256_loadu_pd(a_mat + i);
//                d2 = _mm256_loadu_pd(b_mat + i);
//                res = _mm256_add_pd(d1, d2);
//                _mm256_storeu_pd(res_mat + i, res);
//            }
//            for (int k = total / 4 * 4; k < total; ++k) {
//                res_mat[k] = a_mat[k] + b_mat[k];
//            }
//        }
//        return 0;

        //use omp and AVX
        __m256d d1;
        __m256d d2;
        __m256d res1;
        __m256d res2;
        __m256d res3;
        __m256d res4;
        __m256d res5;
        int unroll = 4;
        int stride = 4 * unroll;
        omp_set_num_threads(16);
        #pragma omp parallel private(d1, d2, res1, res2, res3, res4, res5)
        {
            #pragma omp for
            for (int i = 0; i < total / stride * stride; i += 16) {
                d1 = _mm256_loadu_pd(a_mat + i);
                d2 = _mm256_loadu_pd(b_mat + i);
                res1 = _mm256_add_pd(d1, d2);

                d1 = _mm256_loadu_pd(a_mat + i);
                d2 = _mm256_loadu_pd(b_mat + i);
                res2 = _mm256_add_pd(d1, d2);

                d1 = _mm256_loadu_pd(a_mat + i);
                d2 = _mm256_loadu_pd(b_mat + i);
                res3 = _mm256_add_pd(d1, d2);

                d1 = _mm256_loadu_pd(a_mat + i);
                d2 = _mm256_loadu_pd(b_mat + i);
                res4 = _mm256_add_pd(d1, d2);

                _mm256_storeu_pd(res_mat + i, res1);
                _mm256_storeu_pd(res_mat + i + 4, res2);
                _mm256_storeu_pd(res_mat + i + 8, res3);
                _mm256_storeu_pd(res_mat + i + 12, res4);
            }

            for (int j = total / stride * stride; j < total / 4 * 4; j += 4) {
                d1 = _mm256_loadu_pd(a_mat + j);
                d2 = _mm256_loadu_pd(b_mat + j);
                res5 = _mm256_add_pd(d1, d2);
                _mm256_storeu_pd(res_mat + j, res5);
            }
            for (int k = total / 4 * 4; k < total; ++k) {
                res_mat[k] = a_mat[k] + b_mat[k];
            }
        }
        return 0;
    }
    // dimensions dont match, return nonzero
    return 1;
}

/*
 * Store the result of subtracting mat2 from mat1 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int sub_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    /* TODO: YOUR CODE HERE */
    if (result->rows == mat1->rows && result->cols == mat1->cols
        && result->rows == mat2->rows && result->cols == mat2->cols) {
        int total = result->rows * result->cols;
        double *res_mat = result->data;
        double *a_mat = mat1->data;
        double *b_mat = mat2->data;

//        //naive
//        for (int i = 0; i < total; ++i) {
//            res_mat[i] = a_mat[i] - b_mat[i];
//        }
//        return 0;

        //use omp and AVX
        __m256d d1;
        __m256d d2;
        __m256d res;
        int stride = 4;
        omp_set_num_threads(16);
        #pragma omp parallel private(d1, d2, res)
        {
            #pragma omp for
            for (int i = 0; i < total / stride * stride; i += 4) {
                d1 = _mm256_loadu_pd(a_mat + i);
                d2 = _mm256_loadu_pd(b_mat + i);
                res = _mm256_sub_pd(d1, d2);
                _mm256_storeu_pd(res_mat + i, res);
            }
        }
        return 0;
    }
    // dimensions dont match, return nonzero
    return 1;
}

/*
 * Store the result of multiplying mat1 and mat2 to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 * Remember that matrix multiplication is not the same as multiplying individual elements.
 */
int mul_matrix(matrix *result, matrix *mat1, matrix *mat2) {
    /* TODO: YOUR CODE HERE */
    if (result->rows == mat1->rows && result->cols == mat2->cols && mat1->cols == mat2->rows) {
        if (result != mat1 && result != mat2) { // supports A = AB type multiplication
            fill_matrix(result, 0.0);
            for (int i = 0; i < result->rows; ++i) {
                for (int k = 0; k < mat1->cols; ++k) {
                    for (int j = 0; j < result->cols; ++j) {
                        result->data[i * result->cols + j] +=
                                mat1->data[i * mat1->cols + k] * mat2->data[k * mat2->cols + j];
                    }
                }
            }
            return 0;
        } else {
            matrix *tmp = (matrix*) malloc(sizeof(matrix));
            if(allocate_matrix(&tmp, result->rows, result->cols) == 0) {
                mul_matrix(tmp, mat1, mat2);
                copy_matrix(result, tmp);
                free(tmp->data);
                free(tmp);
                return 0;
            }
        }
    }
    return 1;
}

/*
 * Store the result of raising mat to the (pow)th power to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 * Remember that pow is defined with matrix multiplication, not element-wise multiplication.
 */
int pow_matrix(matrix *result, matrix *mat, int pow) {
    /* TODO: YOUR CODE HERE */
    if (mat->rows != mat->cols || pow < 0) {
        return 1; // only works for square matrices and non negative power
    }

    if (result != mat) { // support A = A^n
        matrix *power_mat = (matrix *) malloc(sizeof(matrix));
        if (allocate_matrix(&power_mat, result->rows, result->cols) == 0) {
            copy_matrix(power_mat, mat);
            set_eye(result);
            while (pow > 0) {
                if ((pow & 0x1) == 1) {
                    mul_matrix(result, result, power_mat);
                }
                pow = pow >> 1;
                if (pow >0) {
                    mul_matrix(power_mat, power_mat, power_mat);
                }
            }
        } else { // allocate error
            return 1;
        }
    } else {
        matrix *tmp = (matrix*) malloc(sizeof(matrix));
        if(allocate_matrix(&tmp, result->rows, result->cols) == 0) {
            pow_matrix(tmp, mat, pow);
            copy_matrix(result, tmp);
            free(tmp->data);
            free(tmp);
        } else { // allocate error
            return 1;
        }
    }
    return 0;
}

/*
 * Store the result of element-wise negating mat's entries to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int neg_matrix(matrix *result, matrix *mat) {
    /* TODO: YOUR CODE HERE */
    if (result->cols != mat->cols || result->rows != mat->rows) {
        return 1;
    }
    int total = result->rows * result->cols;
    for (int i = 0; i < total; ++i) {
        result->data[i] = -1.0 * mat->data[i];
    }
    return 0;
}

/*
 * Store the result of taking the absolute value element-wise to `result`.
 * Return 0 upon success and a nonzero value upon failure.
 */
int abs_matrix(matrix *result, matrix *mat) {
    /* TODO: YOUR CODE HERE */
    if (result->cols != mat->cols || result->rows != mat->rows) {
        return 1;
    }
    int total = result->rows * result->cols;
    double tmp;
    for (int i = 0; i < total; ++i) {
        tmp = mat->data[i];
        result->data[i] = tmp > 0 ? tmp : -1.0 * tmp;
    }
    return 0;
}

