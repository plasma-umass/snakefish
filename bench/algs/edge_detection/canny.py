
import cv2
import numpy as np
from edge_detection.convolve import img_convolve, par_img_convolve
from edge_detection.sobel_filter import sobel_filter, par_sobel_filter
from timeit import default_timer as timer
import porpoise

PI = 180


def gen_gaussian_kernel(k_size, sigma):
    center = k_size // 2
    x, y = np.mgrid[0 - center:k_size - center, 0 - center:k_size - center]
    g = 1 / (2 * np.pi * sigma) * np.exp(-(np.square(x) + np.square(y)) / (2 * np.square(sigma)))
    return g

def task(dst, row_s, row_end, image_col, sobel_theta, sobel_grad, gradient_direction, threshold_low=15, threshold_high=30, weak=128, strong=255):
    for row in range(row_s, row_end):
        for col in range(1, image_col - 1):
            direction = gradient_direction[row, col]
            if (
                0 <= direction < 22.5
                    or 15 * PI / 8 <= direction <= 2 * PI
                    or 7 * PI / 8 <= direction <= 9 * PI / 8
            ):
                W = sobel_grad[row, col - 1]
                E = sobel_grad[row, col + 1]
                if sobel_grad[row, col] >= W and sobel_grad[row, col] >= E:
                    dst[row, col] = sobel_grad[row, col]

            elif (PI / 8 <= direction < 3 * PI / 8) or (9 * PI / 8 <= direction < 11 * PI / 8):
                SW = sobel_grad[row + 1, col - 1]
                NE = sobel_grad[row - 1, col + 1]
                if sobel_grad[row, col] >= SW and sobel_grad[row, col] >= NE:
                    dst[row, col] = sobel_grad[row, col]

            elif (3 * PI / 8 <= direction < 5 * PI / 8) or (11 * PI / 8 <= direction < 13 * PI / 8):
                N = sobel_grad[row - 1, col]
                S = sobel_grad[row + 1, col]
                if sobel_grad[row, col] >= N and sobel_grad[row, col] >= S:
                    dst[row, col] = sobel_grad[row, col]

            elif (5 * PI / 8 <= direction < 7 * PI / 8) or (13 * PI / 8 <= direction < 15 * PI / 8):
                NW = sobel_grad[row - 1, col - 1]
                SE = sobel_grad[row + 1, col + 1]
                if sobel_grad[row, col] >= NW and sobel_grad[row, col] >= SE:
                    dst[row, col] = sobel_grad[row, col]
            """
            High-Low threshold detection. If an edge pixel’s gradient value is higher than the high threshold
            value, it is marked as a strong edge pixel. If an edge pixel’s gradient value is smaller than the high
            threshold value and larger than the low threshold value, it is marked as a weak edge pixel. If an edge
            pixel's value is smaller than the low threshold value, it will be suppressed.
            """
            if dst[row, col] >= threshold_high:
                dst[row, col] = strong
            elif dst[row, col] <= threshold_low:
                dst[row, col] = 0
            else:
                dst[row, col] = weak

    return dst[row_s:row_end]

def par_canny(thr_count, image, threshold_low=15, threshold_high=30, weak=128, strong=255):
    image_row, image_col = image.shape[0], image.shape[1]
    # gaussian_filter
    gaussian_out = par_img_convolve(thr_count, image, gen_gaussian_kernel(9, sigma=1.4))
    # get the gradient and degree by sobel_filter
    sobel_grad, sobel_theta = par_sobel_filter(thr_count, gaussian_out)
    gradient_direction = np.rad2deg(sobel_theta)
    gradient_direction += PI
    dst = np.zeros((image_row, image_col))

    """
    Non-maximum suppression. If the edge strength of the current pixel is the largest compared to the other pixels
    in the mask with the same direction, the value will be preserved. Otherwise, the value will be suppressed.
    """
    portion = int((image_row - 1) / thr_count)
    thr_list = []
    dst_s = 1
    dst_e = portion
    for x in range(thr_count-1):
        thr_list.append(porpoise.Thread(task, dst, dst_s, dst_e, image_col, sobel_theta, sobel_grad, gradient_direction))
        dst_s = dst_e
        dst_e += portion

    task(dst, dst_s, image_row - 1, image_col, sobel_theta, sobel_grad, gradient_direction)

    dst_s = 1
    dst_e = portion
    for thr in thr_list:
        dst[dst_s:dst_e] = thr.join()
        dst_s = dst_e
        dst_e += portion

    """
    Edge tracking. Usually a weak edge pixel caused from true edges will be connected to a strong edge pixel while
    noise responses are unconnected. As long as there is one strong edge pixel that is involved in its 8-connected
    neighborhood, that weak edge point can be identified as one that should be preserved.
    """
    for row in range(1, image_row):
        for col in range(1, image_col):
            if dst[row, col] == weak:
                if 255 in (
                        dst[row, col + 1],
                        dst[row, col - 1],
                        dst[row - 1, col],
                        dst[row + 1, col],
                        dst[row - 1, col - 1],
                        dst[row + 1, col - 1],
                        dst[row - 1, col + 1],
                        dst[row + 1, col + 1],
                ):
                    dst[row, col] = strong
                else:
                    dst[row, col] = 0

    return dst

def canny(image, threshold_low=15, threshold_high=30, weak=128, strong=255):
    image_row, image_col = image.shape[0], image.shape[1]
    # gaussian_filter
    gaussian_out = img_convolve(image, gen_gaussian_kernel(9, sigma=1.4))
    # get the gradient and degree by sobel_filter
    sobel_grad, sobel_theta = sobel_filter(gaussian_out)
    gradient_direction = np.rad2deg(sobel_theta)
    gradient_direction += PI

    dst = np.zeros((image_row, image_col))

    """
    Non-maximum suppression. If the edge strength of the current pixel is the largest compared to the other pixels
    in the mask with the same direction, the value will be preserved. Otherwise, the value will be suppressed.
    """
    task(dst, 1, image_row - 1, image_col, sobel_theta, sobel_grad, gradient_direction)

    """
    Edge tracking. Usually a weak edge pixel caused from true edges will be connected to a strong edge pixel while
    noise responses are unconnected. As long as there is one strong edge pixel that is involved in its 8-connected
    neighborhood, that weak edge point can be identified as one that should be preserved.
    """
    for row in range(1, image_row):
        for col in range(1, image_col):
            if dst[row, col] == weak:
                if 255 in (
                        dst[row, col + 1],
                        dst[row, col - 1],
                        dst[row - 1, col],
                        dst[row + 1, col],
                        dst[row - 1, col - 1],
                        dst[row + 1, col - 1],
                        dst[row - 1, col + 1],
                        dst[row + 1, col + 1],
                ):
                    dst[row, col] = strong
                else:
                    dst[row, col] = 0

    return dst


def main(thr_count, loops, file, series):
    images = [
              r'edge_detection/data/lena.jpg',
              r'edge_detection/data/hdr_sqr.jpg',
              r'edge_detection/data/flower_foveon_sqr.jpg',
              r'edge_detection/data/leaves_iso_200_sqr.jpg',
              r'edge_detection/data/nightshot_iso_1600_sqr.jpg',
              r'edge_detection/data/zone_plate_sqr.jpg',
              r'edge_detection/data/spider_web_sqr.jpg'
              ]
    for y in images:
        image = cv2.imread(y, 0)
        num_time = timer()
        num_time -= num_time
        porp_time = num_time
        for x in range(loops):
            start = timer()

            par_canny_dst = par_canny(thr_count, image)

            porp_time += timer() - start
            # cv2.imshow('canny', par_canny_dst)
            # cv2.waitKey(0)
            start = timer()

            canny_dst = canny(image)

            num_time += timer() - start
            # cv2.imshow('canny', canny_dst)
            # cv2.waitKey(0)

        res = [series, ", edge_detection, ", thr_count, ", ", y, ", ", loops, ", ", round(porp_time, 4), ", ", round(num_time,4), "\n"]
        file.write("".join(str(s) for s in res))
        file.flush()
