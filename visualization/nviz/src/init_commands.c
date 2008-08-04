/*      Alex Shevlakov sixote@yahoo.com 02/2000
 *      function added to handle postgres queries
 */
#include "interface.h"

extern int
Nresize_cmd(),
Nset_background_cmd(),
Nchange_position_cmd(),
Nget_position_cmd(),
Nchange_twist_cmd(),
Nchange_persp_cmd(),
Nchange_exag_cmd(),
Nchange_height_cmd(),
Nget_first_exag_cmd(),
Nget_height_cmd(),
Nfinish_cmd(),
Nlights_cmd(),
Nlight_obj_cmd(),
Nnew_light_cmd(),
Ninit_view_cmd(),
Nlibinit_cmd(),
Nhas_transparency_cmd(),
Ntransp_is_set_cmd(),
Nis_masked_cmd(),
Nget_def_color_cmd(),
Nclear_cmd(),
Nset_cancel_cmd(),
Nunset_cancel_func_cmd(),
Nget_cancel_cmd(),
Nset_draw_cmd(),
Nready_draw_cmd(),
Ndone_draw_cmd(),
Nnew_map_obj_cmd(),
Ninit_view_cmd(),
Nget_to_cmd(),
Nget_from_cmd(),
Nlook_here_cmd(),
Nset_focus_cmd(),
Nset_focus_real_cmd(),
Nset_focus_top_cmd(),
Nset_focus_gui_cmd(),
Nget_focus_gui_cmd(),
Nget_real_position_cmd(),
Nget_focus_cmd(),
Nhas_focus_cmd(),
Nset_focus_map_cmd(),
Nset_focus_state_cmd(),
Nmove_to_cmd(),
Nmove_to_real_cmd(),
Nset_fov_cmd(),
Nget_fov_cmd(),
Nset_twist_cmd(),
Nget_twist_cmd(),
Nget_region_cmd(),
Nget_point_on_surf_cmd(),
Nget_point_on_surf_vect(),
Nget_longdim_cmd(),
Nget_zrange_cmd(),
Nget_zextents_cmd(),
Nget_exag_cmd(),
Nset_exag_cmd(),
Nquick_draw_cmd(),
Nauto_draw_cmd(),
Ndraw_all_together_cmd(),
Nsurf_draw_all_cmd(),
Nvect_draw_all_cmd(),
Nsite_draw_all_cmd(),
Nvol_draw_all_cmd(),
Ndraw_line_on_surf_cmd(),
Ndraw_model_cmd(),
Ndraw_wire_cmd(),
Ndraw_X_cmd(),
Nset_Narrow_cmd(),
Ndraw_Narrow_cmd(),
Nset_ScaleBar_cmd(),
Ndraw_ScaleBar_cmd(),
Ndraw_legend_cmd(),
Ndraw_fringe_cmd(),
Nset_viewport_cmd(),
Ndelete_list_cmd(),
Ndone_draw_cmd(),
Nready_draw_cmd(),
Nget_dist_along_surf_cmd(),
Nget_cat_at_xy_cmd(),
Nget_val_at_xy_cmd(),
Nset_light_to_view_cmd(),
Nset_interp_mode_cmd(),
Nset_tension_cmd(),
Nshowtension_start_cmd(),
Nupdate_tension_cmd(),
Nshowtension_stop_cmd(),
Nupdate_frames_cmd(),
Nset_numsteps_cmd(),
Nclear_keys_cmd(),
Nadd_key_cmd(),
Ndo_framestep_cmd(),
Nshow_path_cmd(),
Nshow_site_cmd(),
Nshow_vect_cmd(),
Nshow_vol_cmd(),
Nshow_lab_cmd(),
Ndelete_key_cmd(),
Nmove_key_cmd(),
Nprint_keys_cmd(),
Nwrite_ppm_cmd(),
Nwrite_tif_cmd(),
Ninit_mpeg_cmd(),
Nwrite_mpeg_frame_cmd(),
Nclose_mpeg_cmd(),
Nstart_zoom_cmd(),
Noff_screen_cmd(),
Ncutplane_obj_cmd(),
Nnew_cutplane_obj_cmd(),
Nnum_cutplane_obj_cmd(),
Nset_current_cutplane_cmd(),
Nget_current_cutplane_cmd(),
Nget_cutplane_list_cmd(),
Nset_fence_color_cmd(),
Nget_fence_color_cmd(),
Nget_xyrange_cmd(),
Nset_SDsurf_cmd(),
Nunset_SDsurf_cmd(),
Nset_SDscale_cmd(),
Nget_surf_list_cmd(),
Nget_vect_list_cmd(),
Nget_site_list_cmd(),
Nget_vol_list_cmd(),
Nsave_3dview_cmd(), Nload_3dview_cmd(), Nset_cancel_func_cmd(),
/*  Tk_Tkspecial_waitCmd(), */
  SetScriptFile_Cmd(),
SetState_Cmd(),
CloseScripting_Cmd(),
ScriptAddString_Cmd(),
Tk_ScaleCmd(),
Tk_SendCmd(),
Nsurf_draw_one_cmd(),
Nvect_draw_one_cmd(),
Nsite_draw_one_cmd(),
Nvol_draw_one_cmd(),
Nliteral_from_logical_cmd(), Nlogical_from_literal_cmd(), Nplace_label_cmd();

extern Tk_Window mainWindow;


/******************************************************/
/* Initialize internal NVIZ commands */
int init_commands(Tcl_Interp * interp, Nv_data * data)
{
    /* Disabled security version of send */
    /*  Tcl_CreateCommand(interp, "send", Tk_SendCmd,
       (ClientData) mainWindow, (void (*)()) NULL); */

    /* Scripting commands */
    Tcl_CreateCommand(interp, "Nv_set_script_file",
		      (Tcl_CmdProc *) SetScriptFile_Cmd,
		      (ClientData) mainWindow, (void (*)())NULL);
    Tcl_CreateCommand(interp, "Nv_set_script_state",
		      (Tcl_CmdProc *) SetState_Cmd, (ClientData) mainWindow,
		      (void (*)())NULL);
    Tcl_CreateCommand(interp, "Nv_close_scripting",
		      (Tcl_CmdProc *) CloseScripting_Cmd,
		      (ClientData) mainWindow, (void (*)())NULL);
    Tcl_CreateCommand(interp, "Nv_script_add_string",
		      (Tcl_CmdProc *) ScriptAddString_Cmd,
		      (ClientData) mainWindow, (void (*)())NULL);

    /* Add the cancel function command */
    Tcl_CreateCommand(interp, "Nset_cancel_func",
		      (Tcl_CmdProc *) Nset_cancel_func_cmd,
		      (ClientData) mainWindow, (void (*)())NULL);
    Tcl_CreateCommand(interp, "Nunset_cancel_func",
		      (Tcl_CmdProc *) Nunset_cancel_func_cmd,
		      (ClientData) mainWindow, (void (*)())NULL);

    /* Add the special tkwait command */
    /* REMOVED 26-Feb-2000 by Philip Warner. Replaced with an Idle handler */
    /*
     *  Tcl_CreateCommand(interp, "tkspecial_wait", Tk_Tkspecial_waitCmd,
     *                  (ClientData) mainWindow, (void (*)()) NULL);
     */

    /* Commands for handling logical names */
    Tcl_CreateCommand(interp, "Nliteral_from_logical",
		      (Tcl_CmdProc *) Nliteral_from_logical_cmd,
		      (ClientData) mainWindow, (void (*)())NULL);
    Tcl_CreateCommand(interp, "Nlogical_from_literal",
		      (Tcl_CmdProc *) Nlogical_from_literal_cmd,
		      (ClientData) mainWindow, (void (*)())NULL);

    /* Commands for generating lists of map objects */
    Tcl_CreateCommand(interp, "Nget_surf_list",
		      (Tcl_CmdProc *) Nget_surf_list_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_vect_list",
		      (Tcl_CmdProc *) Nget_vect_list_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_site_list",
		      (Tcl_CmdProc *) Nget_site_list_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_vol_list",
		      (Tcl_CmdProc *) Nget_vol_list_cmd, data, NULL);

    Tcl_CreateCommand(interp, "Nbackground",
		      (Tcl_CmdProc *) Nset_background_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nresize", (Tcl_CmdProc *) Nresize_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nchange_position",
		      (Tcl_CmdProc *) Nchange_position_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_position",
		      (Tcl_CmdProc *) Nget_position_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nchange_persp",
		      (Tcl_CmdProc *) Nchange_persp_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nchange_twist",
		      (Tcl_CmdProc *) Nchange_twist_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nchange_height",
		      (Tcl_CmdProc *) Nchange_height_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_first_exag",
		      (Tcl_CmdProc *) Nget_first_exag_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_height", (Tcl_CmdProc *) Nget_height_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nchange_exag",
		      (Tcl_CmdProc *) Nchange_exag_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ngl_finish", (Tcl_CmdProc *) Nfinish_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nlights", (Tcl_CmdProc *) Nlights_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nnew_light", (Tcl_CmdProc *) Nnew_light_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ninit_view", (Tcl_CmdProc *) Ninit_view_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nlibinit", (Tcl_CmdProc *) Nlibinit_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nhas_transparency",
		      (Tcl_CmdProc *) Nhas_transparency_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ntransp_is_set",
		      (Tcl_CmdProc *) Ntransp_is_set_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nis_masked", (Tcl_CmdProc *) Nis_masked_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_def_color",
		      (Tcl_CmdProc *) Nget_def_color_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nclear", (Tcl_CmdProc *) Nclear_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nset_cancel", (Tcl_CmdProc *) Nset_cancel_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_cancel", (Tcl_CmdProc *) Nget_cancel_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nset_draw", (Tcl_CmdProc *) Nset_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nready_draw", (Tcl_CmdProc *) Nready_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndone_draw", (Tcl_CmdProc *) Ndone_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nnew_map_obj",
		      (Tcl_CmdProc *) Nnew_map_obj_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_to", (Tcl_CmdProc *) Nget_to_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nget_from", (Tcl_CmdProc *) Nget_from_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nlook_here", (Tcl_CmdProc *) Nlook_here_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nhas_focus", (Tcl_CmdProc *) Nhas_focus_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_focus", (Tcl_CmdProc *) Nget_focus_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus", (Tcl_CmdProc *) Nset_focus_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus_real",
		      (Tcl_CmdProc *) Nset_focus_real_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus_top",
		      (Tcl_CmdProc *) Nset_focus_top_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus_gui",
		      (Tcl_CmdProc *) Nset_focus_gui_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_focus_gui",
		      (Tcl_CmdProc *) Nget_focus_gui_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_real_position",
		      (Tcl_CmdProc *) Nget_real_position_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus_map",
		      (Tcl_CmdProc *) Nset_focus_map_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_focus_state",
		      (Tcl_CmdProc *) Nset_focus_state_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nmove_to", (Tcl_CmdProc *) Nmove_to_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nmove_to_real",
		      (Tcl_CmdProc *) Nmove_to_real_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_fov", (Tcl_CmdProc *) Nset_fov_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nget_fov", (Tcl_CmdProc *) Nget_fov_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nset_twist", (Tcl_CmdProc *) Nset_twist_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_twist", (Tcl_CmdProc *) Nget_twist_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_region", (Tcl_CmdProc *) Nget_region_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_point_on_surf",
		      (Tcl_CmdProc *) Nget_point_on_surf_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_point_on_vect",
		      (Tcl_CmdProc *) Nget_point_on_surf_vect, data, NULL);
    Tcl_CreateCommand(interp, "Nget_longdim",
		      (Tcl_CmdProc *) Nget_longdim_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_zrange", (Tcl_CmdProc *) Nget_zrange_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_zextents",
		      (Tcl_CmdProc *) Nget_zextents_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_exag", (Tcl_CmdProc *) Nget_exag_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nset_exag", (Tcl_CmdProc *) Nset_exag_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nquick_draw", (Tcl_CmdProc *) Nquick_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nauto_draw", (Tcl_CmdProc *) Nauto_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_all",
		      (Tcl_CmdProc *) Ndraw_all_together_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nsurf_draw_all",
		      (Tcl_CmdProc *) Nsurf_draw_all_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nsurf_draw_one",
		      (Tcl_CmdProc *) Nsurf_draw_one_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nvect_draw_all",
		      (Tcl_CmdProc *) Nvect_draw_all_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nvect_draw_one",
		      (Tcl_CmdProc *) Nvect_draw_one_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nsite_draw_all",
		      (Tcl_CmdProc *) Nsite_draw_all_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nsite_draw_one",
		      (Tcl_CmdProc *) Nsite_draw_one_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nvol_draw_all",
		      (Tcl_CmdProc *) Nvol_draw_all_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nvol_draw_one",
		      (Tcl_CmdProc *) Nvol_draw_one_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_line_on_surf",
		      (Tcl_CmdProc *) Ndraw_line_on_surf_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_model", (Tcl_CmdProc *) Ndraw_model_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_wire", (Tcl_CmdProc *) Ndraw_wire_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_X", (Tcl_CmdProc *) Ndraw_X_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Nset_Narrow", (Tcl_CmdProc *) Nset_Narrow_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_Narrow",
		      (Tcl_CmdProc *) Ndraw_Narrow_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_ScaleBar", (Tcl_CmdProc *) Nset_Narrow_cmd, data, NULL);	/* reuse Narrow as it's the same */
    Tcl_CreateCommand(interp, "Ndraw_ScaleBar",
		      (Tcl_CmdProc *) Ndraw_ScaleBar_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_legend",
		      (Tcl_CmdProc *) Ndraw_legend_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndraw_fringe",
		      (Tcl_CmdProc *) Ndraw_fringe_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_viewport",
		      (Tcl_CmdProc *) Nset_viewport_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndelete_list",
		      (Tcl_CmdProc *) Ndelete_list_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Ndone_draw", (Tcl_CmdProc *) Ndone_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nready_draw", (Tcl_CmdProc *) Nready_draw_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nget_dist_along_surf",
		      (Tcl_CmdProc *) Nget_dist_along_surf_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_cat_at_xy",
		      (Tcl_CmdProc *) Nget_cat_at_xy_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_val_at_xy",
		      (Tcl_CmdProc *) Nget_val_at_xy_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_light_to_view",
		      (Tcl_CmdProc *) Nset_light_to_view_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_SDsurf", (Tcl_CmdProc *) Nset_SDsurf_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nunset_SDsurf",
		      (Tcl_CmdProc *) Nunset_SDsurf_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_SDscale",
		      (Tcl_CmdProc *) Nset_SDscale_cmd, data, NULL);

    /* Keyframe Animation */
    Tcl_CreateCommand(interp, "Nset_interp_mode",
		      (Tcl_CmdProc *) Nset_interp_mode_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_tension",
		      (Tcl_CmdProc *) Nset_tension_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nshowtension_start",
		      (Tcl_CmdProc *) Nshowtension_start_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nupdate_tension",
		      (Tcl_CmdProc *) Nupdate_tension_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nshowtension_stop",
		      (Tcl_CmdProc *) Nshowtension_stop_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nupdate_frames",
		      (Tcl_CmdProc *) Nupdate_frames_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_numsteps",
		      (Tcl_CmdProc *) Nset_numsteps_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nclear_keys", (Tcl_CmdProc *) Nclear_keys_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nadd_key", (Tcl_CmdProc *) Nadd_key_cmd, data,
		      NULL);
    Tcl_CreateCommand(interp, "Ndo_framestep",
		      (Tcl_CmdProc *) Ndo_framestep_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nshow_path", (Tcl_CmdProc *) Nshow_path_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nshow_site", (Tcl_CmdProc *) Nshow_site_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nshow_vect", (Tcl_CmdProc *) Nshow_vect_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nshow_vol", (Tcl_CmdProc *) Nshow_vol_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nshow_lab", (Tcl_CmdProc *) Nshow_lab_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ndelete_key", (Tcl_CmdProc *) Ndelete_key_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nmove_key", (Tcl_CmdProc *) Nmove_key_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nprint_keys", (Tcl_CmdProc *) Nprint_keys_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nwrite_ppm", (Tcl_CmdProc *) Nwrite_ppm_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nwrite_tif", (Tcl_CmdProc *) Nwrite_tif_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Ninit_mpeg", (Tcl_CmdProc *) Ninit_mpeg_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nwrite_mpeg_frame",
		      (Tcl_CmdProc *) Nwrite_mpeg_frame_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nclose_mpeg", (Tcl_CmdProc *) Nclose_mpeg_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Nstart_zoom", (Tcl_CmdProc *) Nstart_zoom_cmd,
		      data, NULL);
    Tcl_CreateCommand(interp, "Noff_screen", (Tcl_CmdProc *) Noff_screen_cmd,
		      data, NULL);

    /* Cutplane Junk */
    Tcl_CreateCommand(interp, "Ncutplane_obj",
		      (Tcl_CmdProc *) Ncutplane_obj_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nnew_cutplane_obj",
		      (Tcl_CmdProc *) Nnew_cutplane_obj_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nnum_cutplane_obj",
		      (Tcl_CmdProc *) Nnum_cutplane_obj_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_current_cutplane",
		      (Tcl_CmdProc *) Nset_current_cutplane_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_current_cutplane",
		      (Tcl_CmdProc *) Nget_current_cutplane_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_cutplane_list",
		      (Tcl_CmdProc *) Nget_cutplane_list_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nset_fence_color",
		      (Tcl_CmdProc *) Nset_fence_color_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_fence_color",
		      (Tcl_CmdProc *) Nget_fence_color_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nget_xyrange",
		      (Tcl_CmdProc *) Nget_xyrange_cmd, data, NULL);

    /* Miscellanious */
    Tcl_CreateCommand(interp, "Nsave_3dview",
		      (Tcl_CmdProc *) Nsave_3dview_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nload_3dview",
		      (Tcl_CmdProc *) Nload_3dview_cmd, data, NULL);
    Tcl_CreateCommand(interp, "Nplace_label",
		      (Tcl_CmdProc *) Nplace_label_cmd, data, NULL);

    return (TCL_OK);
}
