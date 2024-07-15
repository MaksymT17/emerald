#include <gtk/gtk.h>
#include <thread>
#include <atomic>
#include <memory>
#include "aquamarine/AmApi.h"
#include <glib.h>
#include "FileProvider.hpp"
// as alternative of cmake compilation:
// example of g++ cmd:
// g++ main.cpp -std=c++14 -o gtk_am_sample `pkg-config gtk+-3.0 --cflags pkg-config gtk+-3.0 --libs` -I aquamarine aquamarine/build/libaquamarine_lib.a && ./gtk_am_sample

#include <cstdlib>
#include <iostream>

constexpr int WINDOW_WIDTH = 960;
constexpr int WINDOW_HEIGHT = 540;

am::AmApi amApi("configuration.csv");

// global variables, used for static callbacks or selection file events
std::string base_image;
GtkWidget *image;
GtkWidget *button, *button2, *btn_auto;
double aspect_ratio;
double scale_ratio_w, scale_ratio_h;
am::analyze::algorithm::DescObjects rect_objs;
double width_o, height_o, width, height;
GdkPixbuf *pixbuf;

void on_min_pixels_changed(GtkRange *range, gpointer data)
{
    auto conf = amApi.getConfiguration();

    gdouble value = gtk_range_get_value(range);
    conf->MinPixelsForObject = static_cast<size_t>(value);
    g_print("Scale value changed to %f\n", value);
    amApi.setConfiguration(conf);
}

void on_step_changed(GtkRange *range, gpointer data)
{
    auto conf = amApi.getConfiguration();

    gdouble value = gtk_range_get_value(range);
    conf->PixelStep = static_cast<size_t>(value);
    g_print("Scale step changed to %f\n", value);
    amApi.setConfiguration(conf);
}

void on_time_limit_changed(GtkRange *range, gpointer data)
{
    auto conf = amApi.getConfiguration();

    gdouble value = gtk_range_get_value(range);
    conf->CalculationTimeLimit = static_cast<double>(value);
    g_print("Scale time lim changed to %f\n", value);
    amApi.setConfiguration(conf);
}

void on_affinity_changed(GtkRange *range, gpointer data)
{
    auto conf = amApi.getConfiguration();

    gdouble value = gtk_range_get_value(range);
    conf->AffinityThreshold = static_cast<size_t>(value);
    g_print("Scale affinity changed to %f\n", value);
    amApi.setConfiguration(conf);
}

void on_threads_mult_changed(GtkRange *range, gpointer data)
{
    auto conf = amApi.getConfiguration();

    gdouble value = gtk_range_get_value(range);
    conf->ThreadsMultiplier = static_cast<double>(value);
    g_print("Scale ThreadsMultiplier changed to %f\n", value);
    amApi.setConfiguration(conf);
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
        }
        else // compare
        {
            std::string to_comapre = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
            rect_objs = amApi.compare(base_image, to_comapre);
            for (auto &rect : rect_objs)
            {
                // draw rectangle on image
                printf("row:%zd col:%zd    row:%zd col:%zd value:%zd\n",
                       rect.getMinHeight(), rect.getLeft(), rect.getMaxHeight(),
                       rect.getRight(), rect.getPixelsCount());
            }
            gtk_button_set_label(GTK_BUTTON(button), "Load Base Image!");
            base_image.clear();
        }
    }
    gtk_widget_destroy(dialog);
}

void draw_rectangle(GtkWidget *widget, cairo_t *cr)
{
    for (auto &rect : rect_objs)
    {
        cairo_set_source_rgb(cr, 0.69, 0.19, 0);
        cairo_set_line_width(cr, 1.0);
        if (aspect_ratio > 0.5625)
        {
            cairo_rectangle(cr, (rect.getLeft()) / scale_ratio_w , (rect.getMinHeight()) / scale_ratio_h,
                            (rect.getRight() - rect.getLeft() + 1) / scale_ratio_w, (rect.getMaxHeight() - rect.getMinHeight() + 1) / scale_ratio_h);
        }
        else
        {
            cairo_rectangle(cr, (rect.getLeft()) / scale_ratio_w + 2, (rect.getMinHeight()) / scale_ratio_h ,
                            (rect.getRight() - rect.getLeft() + 1) / scale_ratio_w, (rect.getMaxHeight() - rect.getMinHeight() + 1) / scale_ratio_h);
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
        if (fileProvider->isNewFileReady(newFile))
        {
            std::string to_comapre = newFile;
            rect_objs = amApi.compare(base_image, to_comapre);
#if defined __APPLE__
            if (rect_objs.size())
            {
                std::string say_text("say Found ");
                say_text.append(std::to_string(rect_objs.size()));
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
    fileProvider = std::make_unique<FileProvider>(base_image);

    isAutomaticActivated = true;
    background_cmp_thread = std::thread(automatic_backgound_comparison);

    return G_SOURCE_REMOVE;
}

gboolean
on_widget_deleted(GtkWidget *widget, GdkEvent *event, gpointer data)
{
    isAutomaticActivated = false;
    background_cmp_thread.join();
    gtk_main_quit();
    return TRUE;
}

int main(int argc, char *argv[])
{
    GtkWidget *window;
    GtkWidget *box;

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

    auto conf = amApi.getConfiguration();

    image = gtk_image_new_from_pixbuf(pixbuf);
    g_signal_connect_after(image, "draw", G_CALLBACK(draw_rectangle), NULL);
    gtk_box_pack_start(GTK_BOX(box), image, FALSE, FALSE, 0);

    button = gtk_button_new_with_label("Load Base Image!");
    g_signal_connect(button, "clicked", G_CALLBACK(open_dialog), (gpointer)image);
    gtk_box_pack_start(GTK_BOX(box), button, FALSE, FALSE, 0);

    btn_auto = gtk_button_new_with_label("Automatic");
    g_signal_connect(btn_auto, "clicked", G_CALLBACK(automatic_search), NULL);
    gtk_box_pack_start(GTK_BOX(box), btn_auto, FALSE, FALSE, 0);

    // min pixs in object
    range_min_pix = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 120.0, 1);
    gtk_scale_add_mark(GTK_SCALE(range_min_pix), 0, GTK_POS_TOP, "MinPixs");
    g_signal_connect(range_min_pix, "value-changed", G_CALLBACK(on_min_pixels_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_time_limit), conf->MinPixelsForObject);
    gtk_box_pack_start(GTK_BOX(box), range_min_pix, FALSE, FALSE, 0);

    // step
    range_step = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 20.0, 2);
    gtk_scale_add_mark(GTK_SCALE(range_step), 0, GTK_POS_TOP, "Step");
    g_signal_connect(range_step, "value-changed", G_CALLBACK(on_step_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_time_limit), conf->PixelStep);
    gtk_box_pack_start(GTK_BOX(box), range_step, FALSE, FALSE, 0);

    // time limit
    range_time_limit = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.1, 60.0, 0.5);
    gtk_scale_add_mark(GTK_SCALE(range_time_limit), 0, GTK_POS_TOP, "TimeLim");
    g_signal_connect(range_time_limit, "value-changed", G_CALLBACK(on_time_limit_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_time_limit), conf->CalculationTimeLimit);
    gtk_box_pack_start(GTK_BOX(box), range_time_limit, FALSE, FALSE, 0);

    // affinity treshold
    range_affinity_treshold = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 1.0, 764, 1);
    gtk_scale_add_mark(GTK_SCALE(range_affinity_treshold), 0, GTK_POS_TOP, "Aff");
    g_signal_connect(range_affinity_treshold, "value-changed", G_CALLBACK(on_affinity_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_affinity_treshold), conf->AffinityThreshold);
    gtk_box_pack_start(GTK_BOX(box), range_affinity_treshold, FALSE, FALSE, 0);

    // threads multiplier
    range_threads_mult = gtk_scale_new_with_range(GTK_ORIENTATION_VERTICAL, 0.1, 60.0, 2);
    gtk_scale_add_mark(GTK_SCALE(range_threads_mult), 0, GTK_POS_TOP, "ThrMult");
    g_signal_connect(range_threads_mult, "value-changed", G_CALLBACK(on_threads_mult_changed), NULL);
    gtk_range_set_value(GTK_RANGE(range_threads_mult), conf->ThreadsMultiplier);
    gtk_box_pack_start(GTK_BOX(box), range_threads_mult, FALSE, FALSE, 0);

    g_signal_connect(G_OBJECT(window), "destroy", G_CALLBACK(gtk_main_quit), NULL);

    gtk_widget_show_all(window);

    gtk_main();
    return 0;
}