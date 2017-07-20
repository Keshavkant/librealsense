// License: Apache 2.0. See LICENSE file in root directory.
// Copyright(c) 2015 Intel Corporation. All Rights Reserved.

#include <librealsense/rs2.hpp>
#include <librealsense/rsutil2.hpp>
#include "example.hpp"

#include <sstream>
#include <iostream>
#include <memory>

using namespace rs2;
using namespace std;


int main(int argc, char * argv[])
{
    context ctx;
    util::device_hub hub(ctx);

    auto finished = false;
    GLFWwindow* win;
    log_to_file(RS2_LOG_SEVERITY_DEBUG);

    while (!finished)
    {
        try
        {
            auto dev = hub.wait_for_device();

            // Configure all supported streams to run at 30 frames per second
            util::config config;
            config.enable_all(preset::best_quality);

            auto stream = config.open(dev);

            syncer_processing_block syncer;
            stream.start(syncer);

            size_t max_frames = 0;

           // black.start(syncer);
            frame_queue queue;
            syncer.start(queue);
            texture_buffer buffers[RS2_STREAM_COUNT];
            bool is_stream_active[RS2_STREAM_COUNT] = {false};
            // Open a GLFW window
            glfwInit();
            ostringstream ss;
            ss << "CPP Capture Example (" << dev.get_info(RS2_CAMERA_INFO_NAME) << ")";

            win = glfwCreateWindow(1280, 720, ss.str().c_str(), nullptr, nullptr);
            glfwMakeContextCurrent(win);

            while (hub.is_connected(dev) && !glfwWindowShouldClose(win))
            {
                int w, h;
                glfwGetFramebufferSize(win, &w, &h);

                auto index = 0;
                auto frames = queue.wait_for_frames();

                max_frames = std::max(max_frames, frames.size());

                //// for consistent visualization, sort frames based on stream type:
                sort(frames.begin(), frames.end(),
                     [](const frame& a, const frame& b) -> bool
                {
                    return a.get_stream_type() < b.get_stream_type();
                });

                auto tiles_horisontal = static_cast<int>(ceil(sqrt(max_frames)));
                auto tiles_vertical = ceil((float)max_frames / tiles_horisontal);
                auto tile_w = static_cast<float>((float)w / tiles_horisontal);
                auto tile_h = static_cast<float>((float)h / tiles_vertical);

                for (auto&& frame : frames)
                {
                    auto stream_type = frame.get_stream_type();
                    buffers[stream_type].upload(frame);
                    is_stream_active[stream_type] = true;
                }

                // Wait for new images
                glfwPollEvents();

                // Clear the framebuffer
                glViewport(0, 0, w, h);
                glClear(GL_COLOR_BUFFER_BIT);

                // Draw the images
                glPushMatrix();
                glfwGetWindowSize(win, &w, &h);
                glOrtho(0, w, h, 0, -1, +1);

                index = 0;
                for (auto i = 0; i< RS2_STREAM_COUNT; i++)
                {
                    auto col_id = index / tiles_horisontal;
                    auto row_id = index % tiles_horisontal;

                    if(is_stream_active[i])
                    {
                        buffers[i].show({ row_id * tile_w, static_cast<float>(col_id * tile_h), tile_w, tile_h }, 1);
                        index++;
                    }
                }

                glPopMatrix();
                glfwSwapBuffers(win);
            }

            if (glfwWindowShouldClose(win))
                finished = true;

        }
        catch (const error & e)
        {
            cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << endl;
        }
        catch (const exception & e)
        {
            cerr << e.what() << endl;
        }
        glfwDestroyWindow(win);
        glfwTerminate();
    }
    return EXIT_SUCCESS;
}
