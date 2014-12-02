#include <gtk/gtk.h>

typedef struct
{
    GtkBuilder *builder;
    gpointer user_data;
} SGlobalData;

gchar *img_name;
gchar *txt_ocr = "test\tbloblo\nbli";
gchar *txt_saved_name;
gchar *txt_saved_path;

GtkTextView *text_view = NULL;
GtkTextBuffer *buffer = NULL;
GtkWidget *dialog_save = NULL;
GtkButton *b_save = NULL;

void callback_about (GtkMenuItem *menuitem, gpointer user_data);
void get_img (GtkFileChooserButton *wigdet, gpointer user_data);
void ocr_text (GtkButton *widget, gpointer user_data);
void save_text( GtkButton *widget, gpointer user_data);
void save_dial (GtkButton *widget, gpointer user_data);


int main(int argc, char *argv [])
{
    GtkWidget *main_window = NULL;
    SGlobalData data;
    GError *error = NULL;
    gchar *filename = NULL;

    gtk_init(&argc, &argv);

    data.builder = gtk_builder_new();

    filename =  g_build_filename ("gui.glade", NULL);

    gtk_builder_add_from_file (data.builder, filename, &error);
    g_free (filename);
    if (error)
    {
        gint code = error->code;
        g_printerr("%s\n", error->message);
        g_error_free (error);
        return code;
    }

    gtk_builder_connect_signals (data.builder, &data);
    
    main_window = GTK_WIDGET(gtk_builder_get_object (data.builder, "window1"));

    gtk_widget_show_all (main_window);

    gtk_main();

    return 0;
}

// about window + credits window
void callback_about (GtkMenuItem *menuitem, gpointer user_data)
{
    SGlobalData *data = (SGlobalData*) user_data;
    GtkWidget *dialog = NULL;

    dialog =  GTK_WIDGET (gtk_builder_get_object (data->builder, "AboutWindow"));
    gtk_dialog_run (GTK_DIALOG (dialog));

    gtk_widget_hide (dialog);
}

// load and print the choosen image, also get the pathname
void get_img(GtkFileChooserButton *widget, gpointer user_data)
{
    SGlobalData *data = (SGlobalData*) user_data;
    GtkFileChooserButton *loader = NULL;
    GtkImage *image = NULL;

    loader = (GtkFileChooserButton*) (gtk_builder_get_object(data->builder, "img_loader"));
    image = (GtkImage*) (gtk_builder_get_object(data->builder, "image1"));

    img_name = (gchar*) gtk_file_chooser_get_filename(  loader );
    g_print("Image path: %s\n", img_name);

    gtk_image_set_from_file( image, (gchar*)img_name);
}
// print the text produce by the ocr (actually just print "test" for now)
void ocr_text (GtkButton *widget, gpointer user_data)
{
    SGlobalData *data = (SGlobalData*) user_data;

    text_view = (GtkTextView*) ( gtk_builder_get_object( data->builder, "textview1")); 
    buffer = gtk_text_view_get_buffer ( text_view );
    
    gtk_text_buffer_set_text ( buffer, txt_ocr, strlen(txt_ocr) );
}

void save_dial (GtkButton *widget, gpointer user_data)
{
    SGlobalData *data = (SGlobalData*) user_data;

    dialog_save = GTK_WIDGET(gtk_builder_get_object(data->builder, "dialog_save"));
    b_save = GTK_BUTTON(gtk_builder_get_object(data->builder, "button_save"));
    
    g_signal_connect_swapped (b_save, "clicked", (GCallback) gtk_widget_hide, dialog_save);

    gtk_dialog_run(GTK_DIALOG(dialog_save));
    gtk_widget_hide(dialog_save); 
}   


void save_text (GtkButton *widget, gpointer user_data)
{
    SGlobalData *data = (SGlobalData*) user_data;
    GtkFileChooser *chooser = NULL;
    chooser = GTK_FILE_CHOOSER(dialog_save);
    GtkEntry *entry = NULL;
    entry = GTK_ENTRY(gtk_builder_get_object(data->builder, "entry1"));
    GtkEntryBuffer *buffer_entry = NULL;

    buffer_entry = gtk_entry_get_buffer(entry);
    txt_saved_name = gtk_entry_buffer_get_text(buffer_entry);
    txt_saved_path = gtk_file_chooser_get_current_folder(chooser);
    
    GtkTextIter iter_start;
    GtkTextIter iter_end;
    
    gtk_text_buffer_get_start_iter ( buffer, &iter_start);
    gtk_text_buffer_get_end_iter ( buffer, &iter_end);
    
    gchar *text2save = gtk_text_buffer_get_text( buffer, &iter_start, &iter_end, TRUE);
    
    gchar *txt_saved = (gchar*) strcat( txt_saved_path, "/");
    txt_saved = (gchar*) strcat( txt_saved, txt_saved_name);
    txt_saved = (gchar*) strcat( txt_saved, ".txt");
    
    g_print("Final name: %s\n", txt_saved);

    FILE *file = NULL;
    file = fopen( txt_saved, "w");

    if ( file == NULL ) {
        g_print("Error in opening .txt file !\n");
        exit(1);
    }

    fputs( text2save, file);

    fclose ( file );

    g_print("Text saved !");

   
}



















