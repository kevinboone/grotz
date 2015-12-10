#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <gtk/gtk.h>
#include <string.h>
#include "dialogs.h"

/*======================================================================
  dialogs_show_pixbuf 
======================================================================*/
void dialogs_show_pixbuf (GtkWindow *parent, GdkPixbuf *pixbuf)
{
   GtkDialog *dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (APPNAME,
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_OK,
        GTK_RESPONSE_NONE,
        NULL));
   GtkImage *image = GTK_IMAGE (gtk_image_new_from_pixbuf (pixbuf));
   
   g_signal_connect_swapped (dialog,
        "response", 
        G_CALLBACK (gtk_widget_destroy),
        dialog);

   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
        GTK_WIDGET (image));
   gtk_widget_show_all (GTK_WIDGET(dialog));
   gtk_dialog_run (dialog);
}

/*======================================================================
  dialogs_show_text
======================================================================*/
void dialogs_show_text (GtkWindow *parent, const char *text)
{
   GtkDialog *dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (APPNAME,
        parent,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_OK,
        GTK_RESPONSE_NONE,
        NULL));

   GtkTextView *tv = GTK_TEXT_VIEW (gtk_text_view_new());
   gtk_text_view_set_wrap_mode (tv, GTK_WRAP_WORD);
   gtk_text_view_set_editable (tv, FALSE);
   GtkScrolledWindow *scroller = GTK_SCROLLED_WINDOW 
     (gtk_scrolled_window_new (NULL, NULL));
   gtk_widget_set_size_request (GTK_WIDGET (scroller), 400, 400);
   gtk_widget_set_size_request (GTK_WIDGET (dialog), 400, 400);
   GtkTextBuffer *buffer = gtk_text_view_get_buffer (tv); 
   GtkTextIter start;
   gtk_text_buffer_get_start_iter (buffer, &start);
   gtk_text_buffer_insert (buffer, &start, text, -1);
   gtk_scrolled_window_add_with_viewport (scroller, GTK_WIDGET (tv));   
   
   g_signal_connect_swapped (dialog,
        "response", 
        G_CALLBACK (gtk_widget_destroy),
        dialog);

   gtk_container_add (GTK_CONTAINER (GTK_DIALOG(dialog)->vbox),
        GTK_WIDGET (scroller));
   gtk_widget_show_all (GTK_WIDGET(dialog));
   gtk_dialog_run (dialog);
}



