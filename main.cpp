#include <gtk/gtk.h>
#include <thread>
#include <atomic>
#include <memory>
#include <glib.h>
#include <cstdlib>
#include <iostream>

#include "FileProvider.hpp"
#include "sh_mem/ProcCommunicator.h"

// as alternative of cmake compilation:
// example of g++ cmd:
// g++ main.cpp -std=c++14 -o gtk_am_sample `pkg-config gtk+-3.0 --cflags pkg-config gtk+-3.0 --libs` -I aquamarine aquamarine/build/libaquamarine_lib.a && ./gtk_am_sample

constexpr int WINDOW_WIDTH = 960;
constexpr int WINDOW_HEIGHT = 540;
size_t client_id{0};

// global variables, used for static callbacks or selection file events
std::string base_image;
GtkWidget *image;
GtkWidget *button, *button2, *btn_auto;
double aspect_ratio;
double scale_ratio_w, scale_ratio_h;
// am::analyze::algorithm::DescObjects rect_objs;
Rect *rect_objs;
size_t rects_count;
double width_o, height_o, width, height;
GdkPixbuf *pixbuf;

const std::string shared_memory_name{"/shared_mem_"};
bool isStopRequested{false}, connectionConfirmed{false};
std::unique_ptr<ProcCommunicator> master = std::make_unique<ProcCommunicator>(true, true, shared_memory_name);
AmConfiguration configuration{75, 10, 1, 50, 5, 10.0};
bool isConfChanged{false};

void on_min_pixels_changed(GtkRange *range, gpointer data)
{
    gdouble value = gtk_range_get_value(range);
    size_t new_val = static_cast<size_t>(value);
    if (new_val != configuration.MinPixelsForObject)
    {
        configuration.MinPixelsForObject = new_val;
        isConfChanged = true;
    }
    g_print("Scale value changed to %f\n", value);
}

void on_step_changed(GtkRange *range, gpointer data)
{
    gdouble value = gtk_range_get_value(range);

    size_t new_val = static_cast<size_t>(value);
    if (new_val != configuration.PixelStep)
    {
        configuration.PixelStep = new_val;
        isConfChanged = true;
    }
    g_print("Scale step changed to %f\n", value);
}

void on_time_limit_changed(GtkRange *range, gpointer data)
{
    gdouble value = gtk_range_get_value(range);

    double new_val = static_cast<double>(value);
    if (new_val != configuration.CalculationTimeLimit)
    {
        configuration.CalculationTimeLimit = new_val;
        isConfChanged = true;
    }
    g_print("Scale time lim changed to %f\n", value);
}

void on_affinity_changed(GtkRange *range, gpointer data)
{
    gdouble value = gtk_range_get_value(range);

    size_t new_val = static_cast<size_t>(value);
    if (new_val != configuration.AffinityThreshold)
    {
        configuration.AffinityThreshold = new_val;
        isConfChanged = true;
    }

    g_print("Scale affinity changed to %f\n", value);
}

void on_threads_mult_changed(GtkRange *range, gpointer data)
{
    gdouble value = gtk_range_get_value(range);

    double new_val = static_cast<double>(value);
    if (new_val != configuration.ThreadsMultiplier)
    {
        configuration.ThreadsMultiplier = new_val;
        isConfChanged = true;
    }
    g_print("Scale ThreadsMultiplier changed to %f\n", value);
}

void set_image_file(const char *fileName)
{
    GdkPixbuf *pixbuf = gdk_pixbuf_new_from_file(fileName, NULL);

    width_o = gdk_pixbuf_get_width(pixbuf);
    height_o = gdk_pixbuf_get_height(pixbuf);

    width = gtk_widget_get_allocated_width(image);
    height = gtk_widget_get_allocated_height(image);
    aspect_ratio = (double)gdk_pixbuf_get_height(pixbuf) / (double)gdk_pixbuf_get_width(pixbuf);

    // debug of dimensions
    g_print("original wh %f %f  window wh %f %f\n", width_o, height_o, width, height);
    if (aspect_ratio > 1.0)
    {
        height = width * aspect_ratio;
    }
    else
    {
        width = height / aspect_ratio;
    }

    scale_ratio_w = width_o / width;
    scale_ratio_h = height_o / height;

    pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);
    gtk_image_set_from_pixbuf(GTK_IMAGE(image), pixbuf);
}

static void open_dialog(GtkWidget *button, gpointer window)
{
    GtkWidget *dialog;

    dialog = GTK_WIDGET(gtk_file_chooser_native_new("Choose a file:", GTK_WINDOW(window), GTK_FILE_CHOOSER_ACTION_OPEN, "_Select", "_Cancel"));

    GtkFileFilter *filter = gtk_file_filter_new();
    gtk_file_filter_set_name(filter, "BMP/JPG files");
    gtk_file_filter_add_mime_type(filter, "image/bmp");
    gtk_file_filter_add_mime_type(filter, "image/jpeg");
    gtk_file_chooser_add_filter(GTK_FILE_CHOOSER(dialog), filter);
    gtk_widget_show_all(dialog);

    gint resp = gtk_native_dialog_run(GTK_NATIVE_DIALOG(dialog));
    if (resp != GTK_RESPONSE_CANCEL)
    {
        g_print("file selected:%s\n", gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));
        set_image_file(gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog)));

        // select base
        if (base_image.empty())
        {
            base_image = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            gtk_button_set_label(GTK_BUTTON(button), "Load & Compare");
            gtk_widget_set_sensitive(btn_auto, true);
        }
        else // compare
        {
            if (isConfChanged)
            {
                printf("CONFIG changed apply new %zu\n", configuration.MinPixelsForObject);
                MessageSetConfig msg_conf{client_id, MessageType::SET_CONFIG, configuration};
                master->send(&msg_conf);
                auto setconfig_resp = master->receive();
                if (setconfig_resp->type == MessageType::SET_CONFIG_OK)
                {
                    g_print("SetConfig OK %zu %zu.", setconfig_resp->id, setconfig_resp->type);
                }
                else
                {
                    g_print("SetConfig Failed. unexpected message type:%zu. Use previous one.", setconfig_resp->type);
                }
                isConfChanged = false;
            }

            std::string to_comapre = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));

            MessageCompareRequest msg{client_id, MessageType::COMPARE_REQUEST, "", ""};
            std::strncpy(msg.base, base_image.c_str(), sizeof(msg.base) - 1);
            msg.base[sizeof(msg.base) - 1] = '\0'; // Ensure null-termination

            std::strncpy(msg.to_compare, to_comapre.c_str(), sizeof(msg.to_compare) - 1);
            msg.to_compare[sizeof(msg.to_compare) - 1] = '\0'; // Ensure null-termination

            g_print("Send compare request %s %s", base_image.c_str(), to_comapre.c_str());
            master->send(&msg);
            Message *resp = master->receive();

            if (resp->type == MessageType::COMPARE_RESULT)
            {
                MessageCompareResult *result_msg = static_cast<MessageCompareResult *>(resp);
                g_print("Received compare result msg_id:%zu type:%zu bytes:%zu.\n", result_msg->id, result_msg->type, result_msg->payload_bytes);
                // std::copy(result_msg->rects, result_msg->rects + 100, rect_objs);
                rect_objs = static_cast<Rect *>(result_msg->payload);
                rects_count = result_msg->payload_bytes / sizeof(Rect);
                g_print("Received compare %zu.\n", rects_count);
            }
            else if (resp->type == MessageType::COMPARE_FAIL)
            {
                g_print("Received COMPARE_FAIL message.\n");
            }
            
            gtk_button_set_label(GTK_BUTTON(button), "Load image");
            base_image.clear();
            gtk_widget_set_sensitive(btn_auto, false);
        }
    }
    gtk_widget_destroy(dialog);
}

void draw_rectangle(GtkWidget *widget, cairo_t *cr)
{
    for (size_t i = 0; i < rects_count; i++)
    {
        const auto &rect = rect_objs[i];
        //g_print("draw_rectangle [%zu] %zu %zu %zu %zu\n", i, rect.l, rect.r, rect.b, rect.t);
        cairo_set_source_rgb(cr, 0.69, 0.19, 0);
        cairo_set_line_width(cr, 1.0);
        if (aspect_ratio > 0.5625)
        {
            cairo_rectangle(cr, (rect.l) / scale_ratio_w, (rect.t) / scale_ratio_h,
                            (rect.r - rect.l + 1) / scale_ratio_w, (rect.t - rect.b + 1) / scale_ratio_h);
        }
        else
        {
            cairo_rectangle(cr, (rect.l) / scale_ratio_w + 2, (rect.b) / scale_ratio_h,
                            (rect.r - rect.l + 1) / scale_ratio_w, (rect.t - rect.b + 1) / scale_ratio_h);
        }
        cairo_stroke(cr);
    }
}

std::thread background_cmp_thread;
bool isAutomaticActivated = false;
std::unique_ptr<FileProvider> fileProvider;

void automatic_backgound_comparison()
{
    while (isAutomaticActivated)
    {
        std::string newFile;
        rects_count = 0;
        if (fileProvider->isNewFileReady(newFile))
        {
            std::string to_comapre = newFile;

            MessageCompareRequest msg{client_id, MessageType::COMPARE_REQUEST, "", ""};
            std::strncpy(msg.base, base_image.c_str(), sizeof(msg.base) - 1);
            msg.base[sizeof(msg.base) - 1] = '\0'; // Ensure null-termination

            std::strncpy(msg.to_compare, to_comapre.c_str(), sizeof(msg.to_compare) - 1);
            msg.to_compare[sizeof(msg.to_compare) - 1] = '\0'; // Ensure null-termination

            g_print("Send compare request %s %s", base_image.c_str(), to_comapre.c_str());
            master->send(&msg);
            Message *resp = master->receive();

            if (resp->type == MessageType::COMPARE_RESULT)
            {
                MessageCompareResult *result_msg = static_cast<MessageCompareResult *>(resp);
                g_print("Received compare result msg_id:%zu type:%zu bytes:%zu.\n", result_msg->id, result_msg->type, result_msg->payload_bytes);
                rect_objs = static_cast<Rect *>(result_msg->payload);
                rects_count = result_msg->payload_bytes / sizeof(Rect);
                g_print("Received compare %zu.\n", rects_count);
            }
            else if (resp->type == MessageType::COMPARE_FAIL)
            {
                g_print("Received COMPARE_FAIL message.\n");
            }
            else
            {
                g_print("Compare Failed. unexpected message type:%zu.", resp->type);
            }

#if defined __APPLE__
            if (rects_count)
            {
                std::string say_text("say Found ");
                say_text.append(std::to_string(rects_count));
                say_text.append(" objects.");
                system(say_text.c_str());
            }
#endif
            set_image_file(to_comapre.c_str());

            g_idle_add((GSourceFunc)gtk_widget_queue_draw, image);

            //  replace base frame for next iteration
            base_image = to_comapre;
        }
        usleep(5000000); // 500ms
    }
}

gboolean automatic_search(gpointer data)
{
    if (!isAutomaticActivated)
    {
        fileProvider = std::make_unique<FileProvider>(base_image);

        isAutomaticActivated = true;
        background_cmp_thread = std::thread(automatic_backgound_comparison);
    }
    else
    {
        isAutomaticActivated = false;
        if (background_cmp_thread.joinable())
            background_cmp_thread.join();
    }
    return G_SOURCE_REMOVE;
}

gboolean on_widget_deleted(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    isAutomaticActivated = false;

    Message msg{client_id, MessageType::DISCONNECT};
    master->send(&msg);

    Message *disconnect_res = master->receive();
    if (disconnect_res->type == MessageType::DISCONNECT_OK)
    {
        std::cout << "Disconnect with running aquamarine completed" << std::endl;
    }

    if (background_cmp_thread.joinable())
        background_cmp_thread.join();

    gtk_main_quit();
    return TRUE;
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *box;

    if(argc == 2)
        client_id = std::atoi(argv[1]);

    GtkWidget *range_min_pix, *range_step, *range_time_limit, *range_affinity_treshold, *range_threads_mult;
    int width = WINDOW_WIDTH;
    int height = WINDOW_HEIGHT;
    gtk_init(&argc, &argv);
    window = gtk_window_new(GTK_WINDOW_TOPLEVEL);
    gtk_window_set_title(GTK_WINDOW(window), "Image and Button Example");
    gtk_container_set_border_width(GTK_CONTAINER(window), 0);
    box = gtk_box_new(GTK_ORIENTATION_HORIZONTAL, 0);
    gtk_container_add(GTK_CONTAINER(window), box);
    g_signal_connect(G_OBJECT(window), "delete-event", G_CALLBACK(on_widget_deleted), NULL);

    pixbuf = gdk_pixbuf_new_from_file("wiki_threaded_bfs.jpg", NULL);
    int image_width = gdk_pixbuf_get_width(pixbuf);
    int image_height = gdk_pixbuf_get_height(pixbuf);
    double aspect_ratio = (double)image_height / (double)image_width;

    if (aspect_ratio > 1.0)
    {
        height = width * aspect_ratio;
    }
    else
    {
        width = height / aspect_ratio;
    }

    pixbuf = gdk_pixbuf_scale_simple(pixbuf, width, height, GDK_INTERP_BILINEAR);

    image = gtk_image_new_from_pixbuf(pixbuf);
    g_signal_connect_after(image, "draw", G_CALLBACK(draw_rectangle), NULL);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Load Base Image!");
    g_signal_connect(button, "clicked", G_CALLBACK(open_dialog), (gpointer)image);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    btn_auto = gtk_button_new_with_label("Automatic");
    g_signal_connect(btn_auto, "clicked", G_CALLBACK(automatic_search), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn_auto, FALSE, FALSE, 0);
    gtk_widget_set_sensitive(btn_auto, false);

    // min pixs in object
    range_min_pix = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 120.0, 1);
    gtk_scale_add_mark(GTK_SCALE(range_min_pix), 0, GTK_POS_TOP, "MinPixs");
    g_signal_connect(range_min_pix, "value-changed", G_CALLBACK(on_min_pixels_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_min_pix), configuration.MinPixelsForObject);
    gtk_box_pack_start(GTK_BOX(box), range_min_pix, FALSE, FALSE, 0);

    // step
    range_step = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 20.0, 2);
    gtk_scale_add_mark(GTK_SCALE(range_step), 0, GTK_POS_TOP, "Step");
    g_signal_connect(range_step, "value-changed", G_CALLBACK(on_step_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_step), configuration.PixelStep);
    gtk_box_pack_start(GTK_BOX(box), range_step, FALSE, FALSE, 0);

    // time limit
    range_time_limit = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.1, 60.0, 0.5);
    gtk_scale_add_mark(GTK_SCALE(range_time_limit), 0, GTK_POS_TOP, "TimeLim");
    g_signal_connect(range_time_limit, "value-changed", G_CALLBACK(on_time_limit_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_time_limit), configuration.CalculationTimeLimit);
    gtk_box_pack_start(GTK_BOX(box), range_time_limit, FALSE, FALSE, 0);

    // affinity treshold
    range_affinity_treshold = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 764, 1);
    gtk_scale_add_mark(GTK_SCALE(range_affinity_treshold), 0, GTK_POS_TOP, "Aff");
    g_signal_connect(range_affinity_treshold, "value-changed", G_CALLBACK(on_affinity_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_affinity_treshold), configuration.AffinityThreshold);
    gtk_box_pack_start(GTK_BOX(box), range_affinity_treshold, FALSE, FALSE, 0);

    // threads multiplier
    range_threads_mult = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.1, 60.0, 2);
    gtk_scale_add_mark(GTK_SCALE(range_threads_mult), 0, GTK_POS_TOP, "ThrMult");
    g_signal_connect(range_threads_mult, "value-changed", G_CALLBACK(on_threads_mult_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_threads_mult), configuration.ThreadsMultiplier);
    gtk_box_pack_start(GTK_BOX(box), range_threads_mult, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);
    g_print("Connecting to aquamarine service. Handshake ...");

    Message msg{client_id, MessageType::HANDSHAKE};
    master->send(&msg);
    auto handshake_resp = master->receive();
    if (handshake_resp->type == MessageType::HANDSHAKE_OK)
    {
        g_print("Connected to aquamarine service. Handshake complete %zu.", handshake_resp->id);
    }
    else
    {
        g_print("Unexpected msg from aquamarine service. Type:%zu.", handshake_resp->type);
        exit(1);
    }

    MessageSetConfig msg_conf{client_id, MessageType::SET_CONFIG, configuration};
    master->send(&msg_conf);
    auto setconfig_resp = master->receive();
    if (setconfig_resp->type == MessageType::SET_CONFIG_OK)
    {
        g_print(" SetConfig OK %zu %zu.", setconfig_resp->id, setconfig_resp->type);
    }
    else
    {
        g_print("Unexpected msg from aquamarine service. Type:%zu.", handshake_resp->type);
        exit(1);
    }

    g_print("Ready to start comparison!");
    gtk_main();

    return 0;
}