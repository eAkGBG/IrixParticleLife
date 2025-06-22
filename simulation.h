#ifndef SIMULATION_H
#define SIMULATION_H

#define NUM_TYPES 6
#define GRID_SIZE 12
#define WORLD_SIZE 2.0f
#define CELL_SIZE (WORLD_SIZE / GRID_SIZE)
#define MAX_INTERACTION_DISTANCE 0.35f
#define MAX_PARTICLES 2000  /* Maximum particles for vertex arrays */

typedef struct {
	float x, y, z;        // 3D coordinates
	float vx, vy, vz;     // 3D velocity
	int type;
} Cell;

typedef struct {
	int count;
	int capacity;         // How many can fit
	Cell *particles;      // Dynamic array of particles
} GridCell;

/* Global variables */
extern GridCell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
extern GridCell work_grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
extern GridCell temp_grid1[GRID_SIZE][GRID_SIZE][GRID_SIZE]; // New temporary grids
extern GridCell temp_grid2[GRID_SIZE][GRID_SIZE][GRID_SIZE];
extern int total_particles;
extern float colors[NUM_TYPES][3];

/* Functions */
void init_grid_with_particles(void);
void init_threads(void);
void update_particles(void);
void cleanup_threads(void);

#endif
