# @Author  : lightXu
# @File    : convolve.py
# @Time    : 2019/7/8 0008 下午 16:13

from cv2 import imread, cvtColor, COLOR_BGR2GRAY, imshow, waitKey
from numpy import array, zeros, ravel, pad, dot, uint8
import porpoise

def task(image, image_array, start, end, row, dst_width, block_size):
    row_s = row
    for i in range(start, end):
        for j in range(0, dst_width):
            window = ravel(image[i:i + block_size[0], j:j + block_size[1]])
            image_array[row, :] = window
            row += 1
    return image_array[row_s:row]


def par_im2col(thr_count, image, block_size):
    rows, cols = image.shape
    dst_height = cols - block_size[1] + 1
    dst_width = rows - block_size[0] + 1
    image_array = zeros((dst_height * dst_width, block_size[1] * block_size[0]))
    portion = int(dst_height / thr_count)
    row_portion = portion * dst_width
    row = 0
    thr_list = []
    start = 0
    end = portion
    for x in range(thr_count - 1):
        thr_list.append(porpoise.Thread(task, image, image_array, start, end, row, dst_width, block_size))
        start = end
        end += portion
        row += row_portion

    task(image, image_array, start, dst_height, row, dst_width, block_size)

    start = 0
    end = row_portion
    for thr in thr_list:
        image_array[start: end] = thr.join()
        start = end
        end += row_portion
    return image_array

def par_img_convolve(thr_count, image, filter_kernel):
    height, width = image.shape[0], image.shape[1]
    k_size = filter_kernel.shape[0]
    pad_size = k_size//2
    # Pads image with the edge values of array.
    image_tmp = pad(image, pad_size, mode='edge')

    # im2col, turn the k_size*k_size pixels into a row and np.vstack all rows
    image_array = par_im2col(thr_count, image_tmp, (k_size, k_size))

    #  turn the kernel into shape(k*k, 1)
    kernel_array = ravel(filter_kernel)
    # reshape and get the dst image
    dst = dot(image_array, kernel_array).reshape(height, width)
    return dst

def im2col(image, block_size):
    rows, cols = image.shape
    dst_height = cols - block_size[1] + 1
    dst_width = rows - block_size[0] + 1
    image_array = zeros((dst_height * dst_width, block_size[1] * block_size[0]))
    row = 0
    for i in range(0, dst_height):
        for j in range(0, dst_width):
            window = ravel(image[i:i + block_size[0], j:j + block_size[1]])
            image_array[row, :] = window
            row += 1
    return image_array


def img_convolve(image, filter_kernel):
    height, width = image.shape[0], image.shape[1]
    k_size = filter_kernel.shape[0]
    pad_size = k_size//2
    # Pads image with the edge values of array.
    image_tmp = pad(image, pad_size, mode='edge')

    # im2col, turn the k_size*k_size pixels into a row and np.vstack all rows
    image_array = im2col(image_tmp, (k_size, k_size))

    #  turn the kernel into shape(k*k, 1)
    kernel_array = ravel(filter_kernel)
    # reshape and get the dst image
    dst = dot(image_array, kernel_array).reshape(height, width)
    return dst


if __name__ == '__main__':
    # read original image
    img = imread(r'./lena.jpg')
    # turn image in gray scale value
    gray = cvtColor(img, COLOR_BGR2GRAY)
    # Laplace operator
    Laplace_kernel = array([[0, 1, 0], [1, -4, 1], [0, 1, 0]])
    out = img_convolve(gray, Laplace_kernel).astype(uint8)
    imshow('Laplacian', out)
    waitKey(0)
