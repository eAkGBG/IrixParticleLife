/*
 * Particle Life - OpenGL Visualization (SGI Octane MXI Optimized)
 */

#include <stdlib.h>
#include <stdio.h>
#include <Xm/Frame.h>  
#include <X11/GLw/GLwMDrawA.h>
#include <X11/keysym.h>
#include <X11/Xutil.h>
#include <GL/glx.h>
#include <GL/glu.h>
#include <math.h>
#include <time.h>
#include "simulation.h"

/* GUI variables */
static Widget toplevel_widget, glx_widget;
static float camera_x = 2.0f, camera_y = 1.5f, camera_z = 2.5f;

/* SGI Octane MXI optimizations */
static GLuint wireframe_display_list = 0;
static int use_display_lists = 1;

/* FPS counter */
static void update_fps_title(void) {
	static int frame_count = 0;
	static time_t last_time = 0;
	static float current_fps = 0.0f;
	char title_buffer[100];
	time_t current_time;
	int gx, gy, gz;
	int total_particles;
	
	total_particles = 0;
	frame_count++;
	current_time = time(NULL);
	
	if (current_time != last_time && last_time != 0) {
		current_fps = (float)frame_count / (float)(current_time - last_time);
		frame_count = 0;
		last_time = current_time;
		
		/* Count total particles directly from grid */
		for (gx = 0; gx < GRID_SIZE; gx++) {
			for (gy = 0; gy < GRID_SIZE; gy++) {
				for (gz = 0; gz < GRID_SIZE; gz++) {
					total_particles += grid[gx][gy][gz].count;
				}
			}
		}
		
		sprintf(title_buffer, "Particle Life SGI - FPS: %.1f - Particles: %d", 
				current_fps, total_particles);
		XtVaSetValues(toplevel_widget, XmNtitle, title_buffer, NULL);
	} else if (last_time == 0) {
		last_time = current_time;
	}
}

/* SGI-optimized wireframe rendering with display lists */
static void create_wireframe_display_list(void) {
	float pos;
	int i;
	
	if (wireframe_display_list != 0) {
		glDeleteLists(wireframe_display_list, 1);
	}
	
	wireframe_display_list = glGenLists(1);
	glNewList(wireframe_display_list, GL_COMPILE);
	
	/* Wireframe cube */
	glColor3f(0.2f, 0.2f, 0.3f);
	glBegin(GL_LINES);
	/* Bottom square */
	glVertex3f(-1.0f, -1.0f, -1.0f); glVertex3f(1.0f, -1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);  glVertex3f(1.0f, -1.0f, 1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);   glVertex3f(-1.0f, -1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);  glVertex3f(-1.0f, -1.0f, -1.0f);
	
	/* Top square */
	glVertex3f(-1.0f, 1.0f, -1.0f); glVertex3f(1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, 1.0f, -1.0f);  glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(1.0f, 1.0f, 1.0f);   glVertex3f(-1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, 1.0f, 1.0f);  glVertex3f(-1.0f, 1.0f, -1.0f);
	
	/* Vertical lines */
	glVertex3f(-1.0f, -1.0f, -1.0f); glVertex3f(-1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, -1.0f);  glVertex3f(1.0f, 1.0f, -1.0f);
	glVertex3f(1.0f, -1.0f, 1.0f);   glVertex3f(1.0f, 1.0f, 1.0f);
	glVertex3f(-1.0f, -1.0f, 1.0f);  glVertex3f(-1.0f, 1.0f, 1.0f);
	glEnd();
	
	/* Grid at bottom */
	glColor3f(0.15f, 0.15f, 0.25f);
	glBegin(GL_LINES);
	for (i = -2; i <= 2; i++) {
		pos = i * 0.5f;
		glVertex3f(pos, -1.0f, -1.0f);
		glVertex3f(pos, -1.0f, 1.0f);
		glVertex3f(-1.0f, -1.0f, pos);
		glVertex3f(1.0f, -1.0f, pos);
	}
	glEnd();
	
	glEndList();
}

/* SGI MXI-optimized particle rendering */
static void draw_particles_optimized(void) {
	int gx, gy, gz, i;
	float dx, dy, dz, camera_distance_sq;
	float brightness;
	Cell *current_particle;
	
	/* Direct rendering without vertex arrays - faster for SGI MXI */
	glPointSize(4.0f);
	glBegin(GL_POINTS);
	
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				for (i = 0; i < grid[gx][gy][gz].count; i++) {
					current_particle = &grid[gx][gy][gz].particles[i];
					
					/* Fast frustum culling */
					if (current_particle->x < -1.2f || current_particle->x > 1.2f ||
						current_particle->y < -1.2f || current_particle->y > 1.2f ||
						current_particle->z < -1.2f || current_particle->z > 1.2f) {
						continue;
					}
					
					/* Fast brightness without sqrt */
					dx = current_particle->x - camera_x;
					dy = current_particle->y - camera_y;
					dz = current_particle->z - camera_z;
					camera_distance_sq = dx*dx + dy*dy + dz*dz;
					
					brightness = (camera_distance_sq < 9.0f) ? 1.0f : 0.7f;
					
					glColor3f(colors[current_particle->type][0] * brightness,
							  colors[current_particle->type][1] * brightness,
							  colors[current_particle->type][2] * brightness);
					glVertex3f(current_particle->x, current_particle->y, current_particle->z);
				}
			}
		}
	}
	
	glEnd();
}

static void draw_scene(void) {
	update_fps_title();
	
	/* SGI MXI-optimized clear operations */
	glClearColor(0.0f, 0.0f, 0.1f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	
	/* Minimal OpenGL state for SGI */
	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);
	
	/* Use display lists for static geometry on SGI */
	if (use_display_lists && wireframe_display_list != 0) {
		glCallList(wireframe_display_list);
	}
	
	/* SGI MXI-compatible particle rendering */
	/* Simpler blending that works well on older SGI hardware */
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA); /* Standard blending for SGI */
	
	/* Enable point smoothing for SGI MXI */
	glEnable(GL_POINT_SMOOTH);
	glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	
	draw_particles_optimized();
	
	glDisable(GL_POINT_SMOOTH);
	glDisable(GL_BLEND);
	glDisable(GL_DEPTH_TEST);
	
	glXSwapBuffers(XtDisplay(toplevel_widget), XtWindow(glx_widget));
}

/* Higher framerate for better performance */
static void game_loop(XtPointer client_data, XtIntervalId *id) {
	/* Avoid unused parameter warnings */
	(void)id;
	
	update_particles();
	draw_scene();
	/* 16ms = ~60 FPS instead of 33ms = 30 FPS */
	XtAppAddTimeOut((XtAppContext)client_data, 16, game_loop, client_data);
}

static void input(Widget w, XtPointer client_data, XtPointer call) {
	char buffer[31];
	KeySym keysym;
	XEvent *event = ((GLwDrawingAreaCallbackStruct *) call)->event;
	
	/* Avoid unused parameter warnings */
	(void)w;
	(void)client_data;
	
	if (event->type == KeyRelease) {
		XLookupString(&event->xkey, buffer, 30, &keysym, NULL);
		switch(keysym) {
			case XK_Escape:
				exit(0);
				break;
			case XK_r:
				init_grid_with_particles();
				break;
			case XK_Left:
				camera_x -= 0.2f;
				break;
			case XK_Right:
				camera_x += 0.2f;
				break;
			case XK_Up:
				camera_y += 0.2f;
				break;
			case XK_Down:
				camera_y -= 0.2f;
				break;
			case XK_Page_Up:
				camera_z -= 0.2f;
				break;
			case XK_Page_Down:
				camera_z += 0.2f;
				break;
		}
		
		glMatrixMode(GL_MODELVIEW);
		glLoadIdentity();
		gluLookAt(camera_x, camera_y, camera_z,
			  0.0f, 0.0f, 0.0f,
			  0.0f, 1.0f, 0.0f);
	}
}

static void resize(Widget w, XtPointer client_data, XtPointer call) {
	GLwDrawingAreaCallbackStruct *call_data = (GLwDrawingAreaCallbackStruct *) call;
	
	/* Avoid unused parameter warnings */
	(void)w;
	(void)client_data;
	
	glViewport(0, 0, call_data->width, call_data->height);
}

static void expose(Widget w, XtPointer client_data, XtPointer call) {
	/* Avoid unused parameter warnings */
	(void)w;
	(void)client_data;
	(void)call;
	
	draw_scene();
}

static int attribs[] = {GLX_RGBA, GLX_DOUBLEBUFFER, GLX_DEPTH_SIZE, 16, None};
static String fallbackResources[] = {
	"*glxwidget*width: 800", "*glxwidget*height: 800",
	NULL
};

/* SGI-optimized initialization */
int main(int argc, char *argv[]) {
	Display *dpy;
	XtAppContext app;
	XVisualInfo *visinfo;
	GLXContext glxcontext;
	Widget toplevel, frame, glxwidget;

	toplevel = XtOpenApplication(&app, "particle_life_sgi", NULL, 0, &argc,
								argv, fallbackResources, applicationShellWidgetClass,
								NULL, 0);
	toplevel_widget = toplevel;

	dpy = XtDisplay(toplevel);
	frame = XmCreateFrame(toplevel, "frame", NULL, 0);
	XtManageChild(frame);

	if(!(visinfo = glXChooseVisual(dpy, DefaultScreen(dpy), attribs)))
		XtAppError(app, "No suitable visual");

	glxwidget = XtVaCreateManagedWidget("glxwidget",
					glwMDrawingAreaWidgetClass, frame, GLwNvisualInfo,
					visinfo, NULL);
	glx_widget = glxwidget;

	XtAddCallback(glxwidget, GLwNexposeCallback, expose, NULL);
	XtAddCallback(glxwidget, GLwNresizeCallback, resize, NULL);
	XtAddCallback(glxwidget, GLwNinputCallback, input, NULL);

	XtRealizeWidget(toplevel);

	glxcontext = glXCreateContext(dpy, visinfo, 0, GL_TRUE);
	GLwDrawingAreaMakeCurrent(glxwidget, glxcontext);
	
	/* SGI MXI-optimized OpenGL settings */
	glMatrixMode(GL_PROJECTION);
	glLoadIdentity();
	gluPerspective(60.0, 1.0, 0.5, 10.0);
	glMatrixMode(GL_MODELVIEW);
	glLoadIdentity();

	gluLookAt(camera_x, camera_y, camera_z,
		  0.0f, 0.0f, 0.0f,
		  0.0f, 1.0f, 0.0f);
	
	/* SGI-specific optimizations */
	glHint(GL_PERSPECTIVE_CORRECTION_HINT, GL_FASTEST);
	glHint(GL_POINT_SMOOTH_HINT, GL_FASTEST);
	glHint(GL_LINE_SMOOTH_HINT, GL_FASTEST);
	
	/* Create display lists for SGI performance */
	create_wireframe_display_list();
	
	srand(time(NULL));
	init_grid_with_particles();
	init_threads();
	
	/* Longer initial delay for SGI initialization */
	XtAppAddTimeOut(app, 200, game_loop, app);
	XtAppMainLoop(app);
	
	/* Cleanup for SGI */
	if (wireframe_display_list != 0) {
		glDeleteLists(wireframe_display_list, 1);
	}
	cleanup_threads();
	return 0;
}
