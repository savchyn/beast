/* BSE - Bedevilled Sound Engine
 * Copyright (C) 1996-1999, 2000-2001 Tim Janik
 *
 * This library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307, USA.
 */
#include        "bsewaverepo.h"

#include        "bsewave.h"


/* --- parameters --- */
enum
{
  PARAM_0,
};


/* --- prototypes --- */
static void	bse_wave_repo_class_init	(BseWaveRepoClass	*class);
static void	bse_wave_repo_init		(BseWaveRepo		*wrepo);
static void	bse_wave_repo_destroy		(BseObject		*object);
static void	bse_wave_repo_set_property	(BseWaveRepo		*wrepo,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void	bse_wave_repo_get_property	(BseWaveRepo		*wrepo,
						 guint			 param_id,
						 GValue			*value,
						 GParamSpec		*pspec);
static void     bse_wave_repo_add_item          (BseContainer		*container,
						 BseItem		*item);
static void     bse_wave_repo_forall_items	(BseContainer		*container,
						 BseForallItemsFunc	 func,
						 gpointer		 data);
static void     bse_wave_repo_remove_item	(BseContainer		*container,
						 BseItem		*item);


/* --- variables --- */
static GTypeClass     *parent_class = NULL;


/* --- functions --- */
BSE_BUILTIN_TYPE (BseWaveRepo)
{
  GType wave_repo_type;

  static const GTypeInfo snet_info = {
    sizeof (BseWaveRepoClass),
    
    (GBaseInitFunc) NULL,
    (GBaseFinalizeFunc) NULL,
    (GClassInitFunc) bse_wave_repo_class_init,
    (GClassFinalizeFunc) NULL,
    NULL /* class_data */,
    
    sizeof (BseWaveRepo),
    0,
    (GInstanceInitFunc) bse_wave_repo_init,
  };
  
  wave_repo_type = bse_type_register_static (BSE_TYPE_SUPER,
					     "BseWaveRepo",
					     "BSE Wave Repository",
					     &snet_info);
  return wave_repo_type;
}

static void
bse_wave_repo_class_init (BseWaveRepoClass *class)
{
  GObjectClass *gobject_class = G_OBJECT_CLASS (class);
  BseObjectClass *object_class = BSE_OBJECT_CLASS (class);
  BseContainerClass *container_class = BSE_CONTAINER_CLASS (class);
  
  parent_class = g_type_class_peek_parent (class);
  
  gobject_class->set_property = (GObjectSetPropertyFunc) bse_wave_repo_set_property;
  gobject_class->get_property = (GObjectGetPropertyFunc) bse_wave_repo_get_property;

  object_class->destroy = bse_wave_repo_destroy;

  container_class->add_item = bse_wave_repo_add_item;
  container_class->remove_item = bse_wave_repo_remove_item;
  container_class->forall_items = bse_wave_repo_forall_items;
}

static void
bse_wave_repo_init (BseWaveRepo *wrepo)
{
  BSE_SUPER (wrepo)->auto_activate = FALSE;
  wrepo->waves = NULL;
  /* we're a singleton within our project */
  BSE_OBJECT_SET_FLAGS (wrepo, BSE_OBJECT_FLAG_FIXED_ULOC);
  g_object_set (wrepo, "uloc", "Wave-Repository", NULL);
}

static void
bse_wave_repo_destroy (BseObject *object)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (object);

  while (wrepo->waves)
    bse_container_remove_item (BSE_CONTAINER (wrepo), wrepo->waves->data);
  
  /* chain parent class' destroy handler */
  BSE_OBJECT_CLASS (parent_class)->destroy (object);
}

static void
bse_wave_repo_set_property (BseWaveRepo *wrepo,
			    guint         param_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wrepo, param_id, pspec);
      break;
    }
}

static void
bse_wave_repo_get_property (BseWaveRepo *wrepo,
			    guint         param_id,
			    GValue       *value,
			    GParamSpec   *pspec)
{
  switch (param_id)
    {
    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (wrepo, param_id, pspec);
      break;
    }
}

static void
bse_wave_repo_add_item (BseContainer *container,
			BseItem      *item)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_WAVE))
    wrepo->waves = g_list_append (wrepo->waves, item);
  else
    g_warning ("BseWaveRepo: cannot hold non-wave item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));
  
  /* chain parent class' add_item handler */
  BSE_CONTAINER_CLASS (parent_class)->add_item (container, item);
}

static void
bse_wave_repo_forall_items (BseContainer      *container,
			    BseForallItemsFunc func,
			    gpointer           data)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);
  GList *list;
  
  list = wrepo->waves;
  while (list)
    {
      BseItem *item;
      
      item = list->data;
      list = list->next;
      if (!func (item, data))
	return;
    }
}

static void
bse_wave_repo_remove_item (BseContainer *container,
			   BseItem      *item)
{
  BseWaveRepo *wrepo = BSE_WAVE_REPO (container);
  
  if (g_type_is_a (BSE_OBJECT_TYPE (item), BSE_TYPE_WAVE))
    wrepo->waves = g_list_remove (wrepo->waves, item);
  else
    g_warning ("BseWaveRepo: cannot hold non-wave item type `%s'",
	       BSE_OBJECT_TYPE_NAME (item));
  
  /* chain parent class' remove_item handler */
  BSE_CONTAINER_CLASS (parent_class)->remove_item (container, item);
}
