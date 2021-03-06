// Licensed GNU LGPL v2.1 or later: http://www.gnu.org/licenses/lgpl.html


/* --- automation setup editor --- */
static GParamSpec*
param_automation_pspec_midi_channel()
{
  static GParamSpec *midi_channel_pspec =
    g_param_spec_ref_sink (sfi_pspec_int ("midi_channel", _("MIDI Channel"),
                                          _("The MIDI Channel from which automation events should be received, 0 designates the default MIDI channel"),
                                          0, 0, 99 /* FIXME: BSE_MIDI_MAX_CHANNELS */, 1, SFI_PARAM_STANDARD ":scale:unprepared"));
  return g_param_spec_ref (midi_channel_pspec);
}

static GParamSpec*
param_automation_pspec_control_type()
{
  static GParamSpec *control_type_pspec =
    g_param_spec_ref_sink (sfi_pspec_choice ("control_type", _("Control Type"),
                                             _("The type of control events used for automation"),
                                             Aida::enum_value_to_string (Bse::MidiControl::CONTINUOUS_16).c_str(),
                                             Bse::choice_values_from_enum<Bse::MidiControl>(),
                                             SFI_PARAM_STANDARD));
  return g_param_spec_ref (control_type_pspec);
}

static void
param_automation_dialog_cancel (GxkDialog *dialog)
{
  g_object_set_data ((GObject*) dialog, "beast-GxkParam", NULL);
  gxk_toplevel_delete (GTK_WIDGET (dialog));
}

static void
param_automation_dialog_ok (GxkDialog *dialog)
{
  GxkParam *param = (GxkParam*) g_object_get_data ((GObject*) dialog, "beast-GxkParam");
  g_object_set_data ((GObject*) dialog, "beast-GxkParam", NULL);
  if (param)
    {
      SfiProxy proxy = bst_param_get_proxy (param);
      GxkParam *param_channel = (GxkParam*) g_object_get_data ((GObject*) dialog, "GxkParam-automation-channel");
      GxkParam *param_control = (GxkParam*) g_object_get_data ((GObject*) dialog, "GxkParam-automation-control");
      gint midi_channel = sfi_value_get_int (&param_channel->value);
      Bse::MidiControl control_type = Aida::enum_value_from_string<Bse::MidiControl> (sfi_value_get_choice (&param_control->value));
      Bse::SourceH source = Bse::SourceH::down_cast (bse_server.from_proxy (proxy));
      source.set_automation (param->pspec->name, midi_channel, control_type);
    }
  gxk_toplevel_delete (GTK_WIDGET (dialog));
}

static void
param_automation_popup_editor (GtkWidget *widget,
                               GxkParam  *param)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  if (proxy)
    {
      static GxkDialog *automation_dialog = NULL;
      if (!automation_dialog)
        {
          automation_dialog = (GxkDialog*) g_object_new (GXK_TYPE_DIALOG, NULL);
          /* configure dialog */
          g_object_set (automation_dialog,
                        "flags", (GXK_DIALOG_HIDE_ON_DELETE |
                                  GXK_DIALOG_PRESERVE_STATE |
                                  GXK_DIALOG_POPUP_POS |
                                  GXK_DIALOG_MODAL),
                        NULL);
          // gxk_dialog_set_sizes (automation_dialog, 550, 300, 600, 320);
          GtkBox *vbox = (GtkBox*) g_object_new (GTK_TYPE_VBOX, "visible", TRUE, "border-width", 5, NULL);
          /* setup parameter: midi_channel */
          GParamSpec *pspec = g_param_spec_ref (param_automation_pspec_midi_channel());
          GxkParam *dialog_param = bst_param_new_value (pspec, NULL, NULL);
          g_param_spec_unref (pspec);
          bst_param_create_gmask (dialog_param, NULL, GTK_WIDGET (vbox));
          g_object_set_data_full ((GObject*) automation_dialog, "GxkParam-automation-channel", dialog_param, (GDestroyNotify) gxk_param_destroy);
          /* setup parameter: control_type */
          pspec = g_param_spec_ref (param_automation_pspec_control_type());
          dialog_param = bst_param_new_value (pspec, NULL, NULL);
          g_param_spec_unref (pspec);
          bst_param_create_gmask (dialog_param, NULL, GTK_WIDGET (vbox));
          g_object_set_data_full ((GObject*) automation_dialog, "GxkParam-automation-control", dialog_param, (GDestroyNotify) gxk_param_destroy);
          /* dialog contents */
          gxk_dialog_set_child (GXK_DIALOG (automation_dialog), GTK_WIDGET (vbox));
          /* provide buttons */
          gxk_dialog_default_action_swapped (automation_dialog, BST_STOCK_OK, (void*) param_automation_dialog_ok, automation_dialog);
          gxk_dialog_action_swapped (automation_dialog, BST_STOCK_CANCEL, (void*) param_automation_dialog_cancel, automation_dialog);
        }
      g_object_set_data ((GObject*) automation_dialog, "beast-GxkParam", param);
      GxkParam *param_channel = (GxkParam*) g_object_get_data ((GObject*) automation_dialog, "GxkParam-automation-channel");
      GxkParam *param_control = (GxkParam*) g_object_get_data ((GObject*) automation_dialog, "GxkParam-automation-control");
      Bse::SourceH source = Bse::SourceH::down_cast (bse_server.from_proxy (proxy));
      sfi_value_set_int (&param_channel->value, source.get_automation_channel (param->pspec->name));
      Bse::MidiControl control_type = source.get_automation_control (param->pspec->name);
      sfi_value_set_choice (&param_control->value, Aida::enum_value_to_string (control_type).c_str());
      gxk_param_apply_value (param_channel); /* update model, auto updates GUI */
      gxk_param_apply_value (param_control); /* update model, auto updates GUI */
      g_object_set_data ((GObject*) widget, "GxkParam-automation-channel", param_channel);
      g_object_set_data ((GObject*) widget, "GxkParam-automation-control", param_control);
      /* setup for proxy */
      bst_window_sync_title_to_proxy (automation_dialog, proxy,
                                      /* TRANSLATORS: this is a dialog title and %s is replaced by an object name */
                                      _("Control Automation: %s"));
      /* cleanup connections to old parent_window */
      if (GTK_WINDOW (automation_dialog)->group)
        gtk_window_group_remove_window (GTK_WINDOW (automation_dialog)->group, GTK_WINDOW (automation_dialog));
      gtk_window_set_transient_for (GTK_WINDOW (automation_dialog), NULL);
      /* setup connections to new parent_window */
      GtkWindow *parent_window = (GtkWindow*) gtk_widget_get_ancestor (widget, GTK_TYPE_WINDOW);
      if (parent_window)
        {
          gtk_window_set_transient_for (GTK_WINDOW (automation_dialog), parent_window);
          if (parent_window->group)
            gtk_window_group_add_window (parent_window->group, GTK_WINDOW (automation_dialog));
        }
      gxk_widget_showraise (GTK_WIDGET (automation_dialog));
    }
}

static void
param_automation_unrequest_focus_space (GtkWidget      *button,  // GTKFIX: GtkButton requests focus space for !CAN_FOCUS
                                        GtkRequisition *requisition)
{
  gint focus_width = 0, focus_pad = 0;
  gtk_widget_style_get (button, "focus-line-width", &focus_width, "focus-padding", &focus_pad, NULL);
  if (requisition->width > 2 * (focus_width + focus_pad) &&
      requisition->height > 2 * (focus_width + focus_pad))
    {
      requisition->width -= 2 * (focus_width + focus_pad);
      requisition->height -= 2 * (focus_width + focus_pad);
    }
}

static GtkWidget*
param_automation_create (GxkParam    *param,
                         const gchar *tooltip,
                         guint        variant)
{
  /* create fake-entry dialog-popup button */
  GtkWidget *widget = (GtkWidget*) g_object_new (GTK_TYPE_EVENT_BOX, NULL);
  gxk_widget_modify_normal_bg_as_base (widget);
  GtkWidget *button = (GtkWidget*) g_object_new (GTK_TYPE_BUTTON,
                                    "can-focus", 0,
                                    "parent", widget,
                                    NULL);
  gxk_widget_modify_normal_bg_as_base (button);
  g_object_connect (button, "signal_after::size-request", param_automation_unrequest_focus_space, button, NULL);
  GtkWidget *label = (GtkWidget*) g_object_new (GTK_TYPE_LABEL,
                                   "label", "88",
                                   "xpad", 2,
                                   "parent", button,
                                   NULL);
  gtk_widget_show_all (widget);
  /* store handles */
  g_object_set_data ((GObject*) widget, "beast-GxkParam", param);
  g_object_set_data ((GObject*) widget, "beast-GxkParam-label", label);
  /* connections */
  g_object_connect (button, "signal::clicked", param_automation_popup_editor, param, NULL);
  return widget;
}

static const SfiChoiceValue*
param_automation_find_choice_value (const gchar *choice,
                                    GParamSpec  *pspec)
{
  SfiChoiceValues cvalues = sfi_pspec_get_choice_values (pspec);
  guint i;
  for (i = 0; i < cvalues.n_values; i++)
    if (sfi_choice_match (cvalues.values[i].choice_ident, choice))
      return &cvalues.values[i];
  return NULL;
}

static void
param_automation_update (GxkParam  *param,
                         GtkWidget *widget)
{
  SfiProxy proxy = bst_param_get_proxy (param);
  gchar *content = NULL, *tip = NULL;
  if (proxy)
    {
      Bse::SourceH source = Bse::SourceH::down_cast (bse_server.from_proxy (proxy));
      const gchar *prefix = "";
      int midi_channel = source.get_automation_channel (param->pspec->name);
      Bse::MidiControl control_type = source.get_automation_control (param->pspec->name);
      GParamSpec *control_pspec = g_param_spec_ref (param_automation_pspec_control_type());
      const SfiChoiceValue *cv = param_automation_find_choice_value (Aida::enum_value_to_string (control_type).c_str(), control_pspec);
      g_param_spec_unref (control_pspec);
      if (control_type >= Bse::MidiControl::CONTINUOUS_0 && control_type <= Bse::MidiControl::CONTINUOUS_31)
        {
          prefix = "c";
          control_type = Bse::MidiControl (int64 (control_type) - int64 (Bse::MidiControl::CONTINUOUS_0));
        }
      else if (control_type >= Bse::MidiControl::CONTROL_0 && control_type <= Bse::MidiControl::CONTROL_127)
        control_type = Bse::MidiControl (int64 (control_type) - int64 (Bse::MidiControl::CONTROL_0));
      else if (control_type == Bse::MidiControl::NONE)
        control_type = Bse::MidiControl (-1);
      else
        control_type = Bse::MidiControl (int64 (control_type) + 10000); /* shouldn't happen */
      if (int64 (control_type) < 0)     /* none */
        {
          content = g_strdup ("--");
          /* TRANSLATORS: %s is substituted with a property name */
          tip = g_strdup_format (_("%s: automation disabled"), g_param_spec_get_nick (param->pspec));
        }
      else if (midi_channel)
        {
          content = g_strdup_format ("%u:%s%02d", midi_channel, prefix, control_type);
          if (cv)
            {
              /* TRANSLATORS: %s is substituted with a property name, %d is substituted with midi control type */
              tip = g_strdup_format (_("%s: automation from MIDI control: %s (MIDI channel: %d)"),
                                     g_param_spec_get_nick (param->pspec),
                                     cv->choice_label ? cv->choice_label : cv->choice_ident,
                                     midi_channel);
            }
        }
      else
        {
          content = g_strdup_format ("%s%02d", prefix, control_type);
          if (cv)
            {
              /* TRANSLATORS: %s is substituted with a property name, %s is substituted with midi control type */
              tip = g_strdup_format (_("%s: automation from MIDI control: %s"),
                                     g_param_spec_get_nick (param->pspec),
                                     cv->choice_label ? cv->choice_label : cv->choice_ident);
            }
        }
    }
  GtkWidget *label = (GtkWidget*) g_object_get_data ((GObject*) widget, "beast-GxkParam-label");
  g_object_set (label,
                "label", content ? content : "--",
                NULL);
  g_free (content);
  gxk_widget_set_tooltip (widget, tip);
  gxk_widget_set_tooltip (gxk_parent_find_descendant (widget, GTK_TYPE_BUTTON), tip);
  g_free (tip);
  Bse::SourceH source = Bse::SourceH::down_cast (bse_server.from_proxy (proxy));
  gtk_widget_set_sensitive (GTK_BIN (widget)->child, proxy && !source.is_prepared());
}

static GxkParamEditor param_automation = {
  { "automation",       N_("Control Automation"), },
  { 0, },
  { "automate",         -5,     TRUE, },        /* options, rating, editing */
  param_automation_create, param_automation_update,
};
