/* BEAST - Bedevilled Audio System
 * Copyright (C) 1998, 1999 Olaf Hoehmann and Tim Janik
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.	 See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU Library General Public
 * License along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include "bstitemview.h"

#include "bstparamview.h"



/* --- prototypes --- */
static void	bst_item_view_class_init	(BstItemViewClass	*klass);
static void	bst_item_view_init		(BstItemView		*item_view,
						 BstItemViewClass	*real_class);
static void	bst_item_view_destroy		(GtkObject		*object);
static void	bst_item_view_finalize		(GtkObject		*object);


/* --- item clist --- */
enum {
  CLIST_SEQID,
  CLIST_NAME,
  CLIST_BLURB,
  CLIST_N_COLUMNS
};
static gchar *clist_titles[CLIST_N_COLUMNS] = {
  "SeqId",
  "Name",
  "Blurb",
};


/* --- static variables --- */
static gpointer		 parent_class = NULL;
static BstItemViewClass *bst_item_view_class = NULL;


/* --- functions --- */
GtkType
bst_item_view_get_type (void)
{
  static GtkType item_view_type = 0;
  
  if (!item_view_type)
    {
      GtkTypeInfo item_view_info =
      {
	"BstItemView",
	sizeof (BstItemView),
	sizeof (BstItemViewClass),
	(GtkClassInitFunc) bst_item_view_class_init,
	(GtkObjectInitFunc) bst_item_view_init,
	/* reserved_1 */ NULL,
	/* reserved_2 */ NULL,
	(GtkClassInitFunc) NULL,
      };
      
      item_view_type = gtk_type_unique (GTK_TYPE_VBOX, &item_view_info);
    }
  
  return item_view_type;
}

static void
bst_item_view_class_init (BstItemViewClass *class)
{
  GtkObjectClass *object_class;
  
  object_class = GTK_OBJECT_CLASS (class);
  
  bst_item_view_class = class;
  parent_class = gtk_type_class (GTK_TYPE_VBOX);
  
  object_class->destroy = bst_item_view_destroy;
  object_class->finalize = bst_item_view_finalize;
  
  class->n_ops = 0;
  class->ops = NULL;
  
  class->operate = NULL;
  class->can_operate = NULL;
}

static void
bst_item_view_init (BstItemView      *item_view,
		    BstItemViewClass *real_class)
{
  item_view->item_type = 0;
  item_view->container = NULL;
  item_view->item_clist = NULL;
  item_view->param_view = NULL;
  item_view->op_widgets = g_new0 (GtkWidget*, real_class->n_ops);
}

static void
bst_item_view_destroy_contents (BstItemView *item_view)
{
  gtk_container_foreach (GTK_CONTAINER (item_view), (GtkCallback) gtk_widget_destroy, NULL);
}

static void
bst_item_view_destroy (GtkObject *object)
{
  BstItemView *item_view;
  
  g_return_if_fail (object != NULL);
  
  item_view = BST_ITEM_VIEW (object);
  
  bst_item_view_destroy_contents (item_view);
  
  bst_item_view_set_container (item_view, NULL);
  
  GTK_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bst_item_view_finalize (GtkObject *object)
{
  BstItemView *item_view;
  
  g_return_if_fail (object != NULL);
  
  item_view = BST_ITEM_VIEW (object);
  
  g_free (item_view->op_widgets);
  
  GTK_OBJECT_CLASS (parent_class)->finalize (object);
}

static void
bst_item_view_item_changed (BstItemView *item_view,
			    BseItem	*item)
{
  GtkCList *clist;
  gint row;
  
  clist = GTK_CLIST (item_view->item_clist);
  row = gtk_clist_find_row_from_data (clist, item);
  if (row >= 0)
    {
      gchar *string;
      
      string = g_strdup_printf ("%03u", bse_item_get_seqid (BSE_ITEM (item)));
      gtk_clist_set_text (clist, row, CLIST_SEQID, string);
      g_free (string);
      gtk_clist_set_text (clist, row, CLIST_NAME, BSE_OBJECT_NAME (item));
      gtk_clist_set_text (clist, row, CLIST_BLURB, bse_object_get_blurb (BSE_OBJECT (item)));
    }
}

static void
bst_item_view_item_param_changed (BstItemView  *item_view,
				  BseParamSpec *pspec,
				  BseItem      *item)
{
  bst_item_view_item_changed (item_view, item);
}

static void
bst_item_view_build_param_view (BstItemView *item_view)
{
  BseContainer *container = item_view->container;
  GList *list, *free_list;
  BseItem *item = NULL;
  
  g_return_if_fail (item_view->param_view == NULL);
  
  free_list = bse_container_list_items (container);
  for (list = free_list; list; list = list->next)
    if (bse_type_is_a (BSE_OBJECT_TYPE (list->data), item_view->item_type))
      item = list->data;
  g_list_free (free_list);
  if (BSE_IS_ITEM (item))
    {
      item_view->param_view = bst_param_view_new (BSE_OBJECT (item));
      gtk_widget_set (item_view->param_view,
		      "signal::destroy", gtk_widget_destroyed, &item_view->param_view,
		      "visible", TRUE,
		      NULL);
      gtk_container_add_with_args (GTK_CONTAINER (item_view), item_view->param_view,
				   "position", 0,
				   "expand", FALSE,
				   "fill", TRUE,
				   "padding", 0,
				   NULL);
      bst_param_view_set_object (BST_PARAM_VIEW (item_view->param_view),
				 (BseObject*) bst_item_view_get_current (item_view));
    }
}

static void
bst_item_view_item_added (BstItemView  *item_view,
			  BseItem      *item,
			  BseContainer *container)
{
  if (BSE_IS_ITEM (item) && bse_type_is_a (BSE_OBJECT_TYPE (item), item_view->item_type))
    {
      static gchar *text[CLIST_N_COLUMNS] = { 0, };
      gint row;
      GtkCList *clist = GTK_CLIST (item_view->item_clist);
      
      bse_object_add_data_notifier (item,
				    "seqid_changed",
				    bst_item_view_item_changed,
				    item_view);
      bse_object_add_data_notifier (item,
				    "param_changed",
				    bst_item_view_item_param_changed,
				    item_view);
      
      row = gtk_clist_insert (clist, -1, text);
      gtk_clist_set_row_data (clist, row, item);
      bst_item_view_item_changed (item_view, item);
      
      if (!item_view->param_view)
	bst_item_view_build_param_view (item_view);
      
      bst_item_view_can_operate (item_view, 0);
    }
}

static void
bst_item_view_item_removed (BstItemView  *item_view,
			    BseItem	 *item,
			    BseContainer *container)
{
  if (BSE_IS_ITEM (item) && bse_type_is_a (BSE_OBJECT_TYPE (item), item_view->item_type))
    {
      gint row;
      GtkCList *clist = GTK_CLIST (item_view->item_clist);
      
      bse_object_remove_notifiers_by_func (item,
					   bst_item_view_item_changed,
					   item_view);
      bse_object_remove_notifiers_by_func (item,
					   bst_item_view_item_param_changed,
					   item_view);
      
      row = gtk_clist_find_row_from_data (clist, item);
      if (row >= 0)
	gtk_clist_remove (clist, row);
      
      bst_update_can_operate (GTK_WIDGET (item_view));
    }
}

void
bst_item_view_set_container (BstItemView  *item_view,
			     BseContainer *new_container)
{
  GList *list;
  BseContainer *container;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  if (new_container)
    g_return_if_fail (BSE_IS_CONTAINER (new_container));
  
  if (item_view->container)
    {
      GList *free_list;
      
      container = item_view->container;
      item_view->container = NULL;
      
      bst_item_view_destroy_contents (item_view);
      
      free_list = bse_container_list_items (BSE_CONTAINER (container));
      for (list = free_list; list; list = list->next)
	if (bse_type_is_a (BSE_OBJECT_TYPE (list->data), item_view->item_type))
	  {
	    bse_object_remove_notifiers_by_func (list->data,
						 bst_item_view_item_changed,
						 item_view);
	    bse_object_remove_notifiers_by_func (list->data,
						 bst_item_view_item_param_changed,
						 item_view);
	  }
      g_list_free (free_list);
      
      bse_object_remove_notifier (container, item_view->item_added_id);
      item_view->item_added_id = 0;
      bse_object_remove_notifier (container, item_view->item_removed_id);
      item_view->item_removed_id = 0;
      bse_object_unref (BSE_OBJECT (container));
    }
  
  item_view->container = new_container;
  if (item_view->container)
    {
      GList *free_list;
      
      container = item_view->container;
      bse_object_ref (BSE_OBJECT (container));
      
      item_view->item_added_id = bse_object_add_data_notifier (container,
							       "item_added",
							       bst_item_view_item_added,
							       item_view);
      item_view->item_removed_id = bse_object_add_data_notifier (container,
								 "item_removed",
								 bst_item_view_item_removed,
								 item_view);
      free_list = bse_container_list_items (BSE_CONTAINER (container));
      for (list = free_list; list; list = list->next)
	if (bse_type_is_a (BSE_OBJECT_TYPE (list->data), item_view->item_type))
	  {
	    bse_object_add_data_notifier (list->data,
					  "seqid_changed",
					  bst_item_view_item_changed,
					  item_view);
	    bse_object_add_data_notifier (list->data,
					  "param_changed",
					  bst_item_view_item_param_changed,
					  item_view);
	  }
      g_list_free (free_list);
      
      bst_item_view_rebuild (item_view);
    }
}

static void
clist_adjust_visibility (GtkCList *clist)
{
  if (clist->selection)
    {
      gint row = GPOINTER_TO_INT (clist->selection->data);
      
      if (gtk_clist_row_is_visible (clist, row) != GTK_VISIBILITY_FULL)
	gtk_clist_moveto (clist, row, -1, 0.5, 0);
    }
}

static void
bst_item_view_selection_changed (BstItemView *item_view)
{
  GtkCList *clist = GTK_CLIST (item_view->item_clist);
  
  if (item_view->param_view)
    bst_param_view_set_object (BST_PARAM_VIEW (item_view->param_view),
			       (BseObject*) bst_item_view_get_current (item_view));

  clist_adjust_visibility (clist);
}

static void
button_action (GtkWidget *widget,
	       gpointer	  op)
{
  while (!BST_IS_ITEM_VIEW (widget))
    widget = widget->parent;
  bst_item_view_operate (BST_ITEM_VIEW (widget), GPOINTER_TO_UINT (op));
}

void
bst_item_view_rebuild (BstItemView *item_view)
{
  BseContainer *container;
  GtkCList *clist;
  GtkWidget *hbox, *any;
  guint i;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  
  bst_item_view_destroy_contents (item_view);
  
  container = item_view->container;
  
  /* action buttons
   */
  any =
    gtk_widget_new (GTK_TYPE_HBOX,
		    "homogeneous", FALSE,
		    "spacing", 0,
		    "border_width", 0,
		    "visible", TRUE,
		    NULL);
  gtk_box_pack_start (GTK_BOX (item_view), any, FALSE, TRUE, 0);
  hbox =
    gtk_widget_new (GTK_TYPE_HBOX,
		    "homogeneous", TRUE,
		    "spacing", 5,
		    "border_width", 5,
		    "visible", TRUE,
		    NULL);
  gtk_box_pack_end (GTK_BOX (any), hbox, FALSE, FALSE, 0);
  
  for (i = 0; i < BST_ITEM_VIEW_GET_CLASS (item_view)->n_ops; i++)
    {
      BstItemViewOp *bop = BST_ITEM_VIEW_GET_CLASS (item_view)->ops + i;
      
      item_view->op_widgets[i] =
	gtk_widget_new (GTK_TYPE_BUTTON,
			"label", bop->op_name,
			"visible", TRUE,
			"signal::clicked", button_action, GUINT_TO_POINTER (bop->op),
			"signal::destroy", gtk_widget_destroyed, &item_view->op_widgets[i],
			"parent", hbox,
			NULL);
    }
  
  /* item list
   */
  item_view->item_clist =
    gtk_widget_new (GTK_TYPE_CLIST,
		    "n_columns", CLIST_N_COLUMNS,
		    "selection_mode", GTK_SELECTION_BROWSE,
		    "titles_active", FALSE,
		    "border_width", 5,
		    "signal::destroy", gtk_widget_destroyed, &item_view->item_clist,
		    "object_signal::select_row", bst_item_view_selection_changed, item_view,
		    "signal_after::size_allocate", clist_adjust_visibility, NULL,
		    "signal_after::map", clist_adjust_visibility, NULL,
		    "visible", TRUE,
		    "parent", gtk_widget_new (GTK_TYPE_SCROLLED_WINDOW,
					      "visible", TRUE,
					      "hscrollbar_policy", GTK_POLICY_AUTOMATIC,
					      "vscrollbar_policy", GTK_POLICY_AUTOMATIC,
					      "parent", item_view,
					      /* "parent", gtk_widget_new (GTK_TYPE_FRAME,
						 "visible", TRUE,
						 "label", "Item List",
						 "parent", item_view,
						 "border_width", 5,
						 NULL),
					      */
					      NULL),
		    NULL);
  clist = GTK_CLIST (item_view->item_clist);
  gtk_clist_set_column_title (clist, CLIST_SEQID, clist_titles[CLIST_SEQID]);
  gtk_clist_set_column_title (clist, CLIST_NAME, clist_titles[CLIST_NAME]);
  gtk_clist_set_column_title (clist, CLIST_BLURB, clist_titles[CLIST_BLURB]);
  gtk_clist_set_column_auto_resize (clist, CLIST_NAME, TRUE);
  gtk_clist_column_titles_show (clist);
  gtk_clist_column_titles_passive (clist);
  
  /* param view
   */
  bst_item_view_build_param_view (item_view);
  
  bst_item_view_update (item_view);
  bst_item_view_can_operate (item_view, 0);
}

void
bst_item_view_update (BstItemView *item_view)
{
  GList *list;
  GtkCList *clist;
  BseContainer *container;
  GList *free_list;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  
  container = item_view->container;
  clist = GTK_CLIST (item_view->item_clist);
  
  gtk_clist_freeze (clist);
  gtk_clist_clear (clist);
  
  free_list = bse_container_list_items (BSE_CONTAINER (container));
  for (list = free_list; list; list = list->next)
    if (bse_type_is_a (BSE_OBJECT_TYPE (list->data), item_view->item_type))
      {
	static gchar *text[CLIST_N_COLUMNS] = { 0, };
	gint row;
	
	row = gtk_clist_insert (clist, 0, text);
	gtk_clist_set_row_data (clist, row, list->data);
	bst_item_view_item_changed (item_view, list->data);
      }
  g_list_free (free_list);
  
  gtk_clist_thaw (clist);
  
  /* update item_view->param_view */
  bst_item_view_selection_changed (item_view);
}

void
bst_item_view_select (BstItemView *item_view,
		      BseItem	  *item)
{
  GtkCList *clist;
  gint row;
  
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  g_return_if_fail (BSE_IS_ITEM (item));
  g_return_if_fail (BSE_ITEM (item)->container == BSE_ITEM (item_view->container));
  
  clist = GTK_CLIST (item_view->item_clist);
  row = gtk_clist_find_row_from_data (clist, item);
  if (row >= 0)
    {
      gtk_clist_freeze (clist);
      gtk_clist_undo_selection (clist);
      gtk_clist_unselect_all (clist);
      gtk_clist_select_row (clist, row, 0);
      gtk_clist_thaw (clist);
    }
}

BseItem*
bst_item_view_get_current (BstItemView *item_view)
{
  BseItem *item = NULL;
  GtkCList *clist;
  
  g_return_val_if_fail (BST_IS_ITEM_VIEW (item_view), NULL);
  
  clist = GTK_CLIST (item_view->item_clist);
  
  if (clist->selection)
    item = gtk_clist_get_row_data (clist, GPOINTER_TO_INT (clist->selection->data));
  
  if (item)
    g_return_val_if_fail (BSE_IS_ITEM (item), NULL);
  
  return item;
}

void
bst_item_view_operate (BstItemView *item_view,
		       BstOps	    op)
{
  g_return_if_fail (BST_IS_ITEM_VIEW (item_view));
  g_return_if_fail (bst_item_view_can_operate (item_view, op));
  
  gtk_widget_ref (GTK_WIDGET (item_view));
  
  BST_ITEM_VIEW_GET_CLASS (item_view)->operate (item_view, op);
  
  bst_update_can_operate (GTK_WIDGET (item_view));
  
  gtk_widget_unref (GTK_WIDGET (item_view));
}

gboolean
bst_item_view_can_operate (BstItemView *item_view,
			   BstOps	op)
{
  gboolean can_do;
  guint i;
  
  g_return_val_if_fail (BST_IS_ITEM_VIEW (item_view), FALSE);
  
  gtk_widget_ref (GTK_WIDGET (item_view));
  
  if (BST_ITEM_VIEW_GET_CLASS (item_view)->operate &&
      BST_ITEM_VIEW_GET_CLASS (item_view)->can_operate)
    can_do = BST_ITEM_VIEW_GET_CLASS (item_view)->can_operate (item_view, op);
  else
    can_do = FALSE;
  
  /* update action buttons */
  for (i = 0; i < BST_ITEM_VIEW_GET_CLASS (item_view)->n_ops; i++)
    {
      BstItemViewOp *bop = BST_ITEM_VIEW_GET_CLASS (item_view)->ops + i;
      
      if (bop->op == op &&
	  item_view->op_widgets[i])
	{
	  gtk_widget_set_sensitive (item_view->op_widgets[i], can_do);
	  break;
	}
    }
  
  gtk_widget_unref (GTK_WIDGET (item_view));
  
  return can_do;
}
