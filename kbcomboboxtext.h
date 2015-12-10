/* GTK - The GIMP Toolkit
 *
 * Copyright (C) 2010 Christian Dywan
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library. If not, see <http://www.gnu.org/licenses/>.
 */

#if defined(GTK_DISABLE_SINGLE_INCLUDES) && !defined (__GTK_H_INSIDE__) && !defined (GTK_COMPILATION)
#error "Only <gtk/gtk.h> can be included directly."
#endif

#ifndef __KB_COMBO_BOX_TEXT_H__
#define __KB_COMBO_BOX_TEXT_H__

#include <gtk/gtkcombobox.h>

G_BEGIN_DECLS

#define KB_TYPE_COMBO_BOX_TEXT                 (kb_combo_box_text_get_type ())
#define KB_COMBO_BOX_TEXT(obj)                 (G_TYPE_CHECK_INSTANCE_CAST ((obj), KB_TYPE_COMBO_BOX_TEXT, KBComboBoxText))
#define KB_COMBO_BOX_TEXT_CLASS(klass)         (G_TYPE_CHECK_CLASS_CAST ((klass), KB_TYPE_COMBO_BOX_TEXT, KBComboBoxTextClass))
#define KB_IS_COMBO_BOX_TEXT(obj)              (G_TYPE_CHECK_INSTANCE_TYPE ((obj), KB_TYPE_COMBO_BOX_TEXT))
#define KB_IS_COMBO_BOX_TEXT_CLASS(klass)      (G_TYPE_CHECK_CLASS_TYPE ((klass), KB_TYPE_COMBO_BOX_TEXT))
#define KB_COMBO_BOX_TEXT_GET_CLASS(obj)       (G_TYPE_INSTANCE_GET_CLASS ((obj), KB_TYPE_COMBO_BOX_TEXT, KBComboBoxTextClass))

typedef struct _KBComboBoxText             KBComboBoxText;
typedef struct _KBComboBoxTextPrivate      KBComboBoxTextPrivate;
typedef struct _KBComboBoxTextClass        KBComboBoxTextClass;

struct _KBComboBoxText
{
  /* <private> */
  GtkComboBox parent_instance;

  KBComboBoxTextPrivate *priv;
};

struct _KBComboBoxTextClass
{
  GtkComboBoxClass parent_class;

  /* Padding for future expansion */
  void (*_gtk_reserved1) (void);
  void (*_gtk_reserved2) (void);
  void (*_gtk_reserved3) (void);
  void (*_gtk_reserved4) (void);
};

GType         kb_combo_box_text_get_type        (void) G_GNUC_CONST;
GtkWidget*    kb_combo_box_text_new             (void);
GtkWidget*    kb_combo_box_text_new_with_entry  (void);
void          kb_combo_box_text_append_text     (KBComboBoxText     *combo_box,
                                                  const gchar         *text);
void          kb_combo_box_text_insert_text     (KBComboBoxText     *combo_box,
                                                  gint                 position,
                                                  const gchar         *text);
void          kb_combo_box_text_prepend_text    (KBComboBoxText     *combo_box,
                                                  const gchar         *text);
void          kb_combo_box_text_remove          (KBComboBoxText     *combo_box,
                                                  gint                 position);
gchar        *kb_combo_box_text_get_active_text (KBComboBoxText     *combo_box);


G_END_DECLS

#endif /* __KB_COMBO_BOX_TEXT_H__ */
