#include <clutter/clutter.h>
#include <math.h>
#include <errno.h>
#include <stdlib.h>

#define TRAILS 0
#define NHANDS  6
#define RADIUS  ((CLUTTER_STAGE_WIDTH()+CLUTTER_STAGE_HEIGHT())/6)

typedef struct SuperOH
{
  ClutterActor *hand[NHANDS], *bgtex;
  ClutterGroup   *group;
  GdkPixbuf      *bgpixb;

} SuperOH; 

void
screensaver_setup (void)
{
  Window         remote_xwindow;
  const char    *preview_xid;
  gboolean       foreign_success = FALSE;

  preview_xid = g_getenv ("XSCREENSAVER_WINDOW");

  if (preview_xid != NULL) 
    {
      char *end;
      remote_xwindow = (Window) strtoul (preview_xid, &end, 0);

      if ((remote_xwindow != 0) && (end != NULL) && 
	  ((*end == ' ') || (*end == '\0')) &&
	  ((remote_xwindow < G_MAXULONG) || (errno != ERANGE))) 
	{
	  foreign_success = clutter_stage_set_xwindow_foreign 
	    (CLUTTER_STAGE(clutter_stage_get_default()), remote_xwindow);
        }
    }

  if (!foreign_success)
    clutter_actor_set_size (clutter_stage_get_default(), 800, 600);
}

/* input handler */
void 
input_cb (ClutterStage *stage, 
	  ClutterEvent *event,
	  gpointer      data)
{
  SuperOH *oh = (SuperOH *)data;

  if (event->type == CLUTTER_BUTTON_PRESS)
    {
      ClutterButtonEvent *bev = (ClutterButtonEvent *) event;
      ClutterActor *e;

      g_print ("*** button press event (button:%d) ***\n",
	       bev->button);

      e = clutter_stage_get_actor_at_pos (stage, 
		                            clutter_button_event_x (event),
					    clutter_button_event_y (event));

      if (e)
	clutter_actor_hide(e);
    }
  else if (event->type == CLUTTER_KEY_PRESS)
    {
      ClutterKeyEvent *kev = (ClutterKeyEvent *) event;

      g_print ("*** key press event (key:%c) ***\n",
	       clutter_key_event_symbol (kev));
      
      if (clutter_key_event_symbol (kev) == CLUTTER_q)
	clutter_main_quit ();
    }
}


/* Timeline handler */
void
frame_cb (ClutterTimeline *timeline, 
	  gint             frame_num, 
	  gpointer         data)
{
  SuperOH        *oh = (SuperOH *)data;
  ClutterActor *stage = clutter_stage_get_default ();
  gint            i;

#if TRAILS
  oh->bgpixb = clutter_stage_snapshot (CLUTTER_STAGE (stage),
				       0, 0,
				       CLUTTER_STAGE_WIDTH(),
				       CLUTTER_STAGE_HEIGHT());
  clutter_texture_set_pixbuf (CLUTTER_TEXTURE (oh->bgtex), oh->bgpixb);
  g_object_unref (G_OBJECT (oh->bgpixb));
  g_object_unref (stage);
#endif

  /* Rotate everything clockwise about stage center*/
  clutter_actor_rotate_z (CLUTTER_ACTOR(oh->group),
			    frame_num,
			    CLUTTER_STAGE_WIDTH()/2,
			    CLUTTER_STAGE_HEIGHT()/2);
  for (i = 0; i < NHANDS; i++)
    {
      /* rotate each hand around there centers */
      clutter_actor_rotate_z (oh->hand[i],
				- 6.0 * frame_num,
				clutter_actor_get_width (oh->hand[i])/2,
				clutter_actor_get_height (oh->hand[i])/2);
    }

  /*
  clutter_actor_rotate_x (CLUTTER_ACTOR(oh->group),
			    75.0,
			    CLUTTER_STAGE_HEIGHT()/2, 0);
  */
}


int
main (int argc, char *argv[])
{
  ClutterTimeline *timeline;
  ClutterActor  *stage;
  ClutterColor     stage_color = { 0x61, 0x64, 0x8c, 0xff };
  GdkPixbuf       *pixbuf;
  SuperOH         *oh;
  gint             i;

  clutter_init (&argc, &argv);

  stage = clutter_stage_get_default ();

  pixbuf = gdk_pixbuf_new_from_file ("redhand.png", NULL);

  if (!pixbuf)
    g_error("pixbuf load failed");

  /* Set our stage (window) size */
  // clutter_actor_set_size (stage, WINWIDTH, WINHEIGHT);

  /* and its background color */

  screensaver_setup ();

  clutter_stage_set_color (CLUTTER_STAGE (stage),
		           &stage_color);

  oh = g_new(SuperOH, 1);

#if TRAILS
  oh->bgtex = clutter_texture_new();
  clutter_actor_set_size (oh->bgtex, 
			  CLUTTER_STAGE_WIDTH(), CLUTTER_STAGE_HEIGHT());
  clutter_actor_set_opacity (oh->bgtex, 0x99);
  clutter_group_add (CLUTTER_GROUP (stage), oh->bgtex);
#endif

  /* create a new group to hold multiple actors in a group */
  oh->group = clutter_group_new();
  
  for (i = 0; i < NHANDS; i++)
    {
      gint x, y, w, h;

      /* Create a texture from pixbuf, then clone in to same resources */
      if (i == 0)
       oh->hand[i] = clutter_texture_new_from_pixbuf (pixbuf);
     else
       oh->hand[i] = clutter_clone_texture_new (CLUTTER_TEXTURE(oh->hand[0]));

      /* Place around a circle */
      w = clutter_actor_get_width (oh->hand[0]);
      h = clutter_actor_get_height (oh->hand[0]);

      x = CLUTTER_STAGE_WIDTH() / 2  
               + RADIUS * cos (i * M_PI / (NHANDS/2)) - w/2;
      y = CLUTTER_STAGE_HEIGHT() / 2 
               + RADIUS * sin (i * M_PI / (NHANDS/2)) - h/2;

      clutter_actor_set_position (oh->hand[i], x, y);

      /* Add to our group group */
      clutter_group_add (oh->group, oh->hand[i]);
    }

  /* Add the group to the stage */
  clutter_group_add (CLUTTER_GROUP (stage), CLUTTER_ACTOR(oh->group));

  /* Show everying ( and map window ) */
  clutter_group_show_all (oh->group);
  clutter_group_show_all (CLUTTER_GROUP (stage));

  g_signal_connect (stage, "button-press-event",
		    G_CALLBACK (input_cb), 
		    oh);
  g_signal_connect (stage, "key-release-event",
		    G_CALLBACK (input_cb),
		    oh);

  /* Create a timeline to manage animation */
  timeline = clutter_timeline_new (360, 90); /* num frames, fps */
  g_object_set(timeline, "loop", TRUE, 0);   /* have it loop */

  /* fire a callback for frame change */
  g_signal_connect(timeline, "new-frame",  G_CALLBACK (frame_cb), oh);

  /* and start it */
  clutter_timeline_start (timeline);

  clutter_main();

  g_object_unref (stage);

  return 0;
}
