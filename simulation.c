#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <time.h>
#include <pthread.h>
#include "simulation.h"

/* Global variables */
GridCell grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
GridCell work_grid[GRID_SIZE][GRID_SIZE][GRID_SIZE];
GridCell temp_grid1[GRID_SIZE][GRID_SIZE][GRID_SIZE]; 
GridCell temp_grid2[GRID_SIZE][GRID_SIZE][GRID_SIZE];
int total_particles = 720;

/* Pthread variables */
static pthread_t thread1, thread2;
static pthread_barrier_t barrier;
static volatile int threads_running = 0;

float colors[NUM_TYPES][3] = {
	{0.3f, 1.0f, 0.3f},  /* Green */
	{1.0f, 0.3f, 0.3f},  /* Red */  
	{1.0f, 1.0f, 0.3f},  /* Yellow */
	{0.3f, 0.3f, 1.0f},  /* Blue */
	{0.3f, 1.0f, 1.0f},  /* Cyan */
	{1.0f, 0.3f, 1.0f}   /* Magenta */
};

/* Attraction matrix - enhanced reactions on green particles */
static float attraction[NUM_TYPES][NUM_TYPES] = {
	{ 0.85f, -0.70f,  0.95f, -0.50f, 0.75f, -0.80f},  /* Green: Strong self-attraction, flees red/blue/magenta, hunts yellow/cyan */
	{-0.85f,  0.53f, -0.53f, -0.84f, -0.23f, 0.40f},  /* Red: Flees STRONGLY from green (was 0.17f) */
	{-0.90f, -0.91f, -0.41f,  0.91f,  0.46f, -0.17f},  /* Yellow: Flees AGGRESSIVELY from green (was -0.40f) */
	{-0.80f, -0.16f,  0.51f, -0.78f, -0.35f, 0.23f},  /* Blue: Flees STRONGLY from green (was -0.53f) */
	{ 0.80f, -0.29f,  0.52f, -0.40f, -0.17f, 0.58f},  /* Cyan: Attracted STRONGLY to green (was 0.35f) */
	{-0.85f,  0.35f, -0.23f,  0.17f,  0.40f, -0.29f}   /* Magenta: Flees STRONGLY from green (was -0.46f) */
};

/* Helper function for periodic boundary conditions - calculates shortest distance with wraparound */
static void calc_periodic_distance(float x1, float y1, float z1, float x2, float y2, float z2, 
								   float *dx, float *dy, float *dz) {
	/* Calculate basic distance */
	*dx = x1 - x2;
	*dy = y1 - y2;
	*dz = z1 - z2;
	
	/* Check wraparound for X-axis (world size = 2.0f, from -1.0 to +1.0) */
	if (*dx > 1.0f) *dx -= 2.0f;        /* Too large positive difference - check other side */
	else if (*dx < -1.0f) *dx += 2.0f;  /* Too large negative difference - check other side */
	
	/* Check wraparound for Y-axis */
	if (*dy > 1.0f) *dy -= 2.0f;
	else if (*dy < -1.0f) *dy += 2.0f;
	
	/* Check wraparound for Z-axis */
	if (*dz > 1.0f) *dz -= 2.0f;
	else if (*dz < -1.0f) *dz += 2.0f;
}

/* Helper function to convert world coordinate to grid index */
static int coord_to_grid(float coord) {
	int index = (int)((coord + 1.0f) / CELL_SIZE);
	if (index < 0) index = 0;
	if (index >= GRID_SIZE) index = GRID_SIZE - 1;
	return index;
}

/* Simple function to add particle to grid */
static void add_particle_to_grid(GridCell *cell, Cell particle) {
	int new_capacity;
	Cell *new_particles;
	
	/* Check if we need more space */
	if (cell->count >= cell->capacity) {
		new_capacity = cell->capacity == 0 ? 16 : cell->capacity * 2;
		new_particles = (Cell*)realloc(cell->particles, new_capacity * sizeof(Cell));
		if (new_particles) {
			cell->particles = new_particles;
			cell->capacity = new_capacity;
		} else {
			printf("CRITICAL ERROR: Could not allocate memory!\n");
			return;
		}
	}
	
	cell->particles[cell->count] = particle;
	cell->count++;
}

void init_grid_with_particles(void) {
	int gx, gy, gz;
	int particles_created;
	Cell new_particle;
	
	particles_created = 0;
	
	/* Clear all grids and ensure particles pointers are NULL */
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				grid[gx][gy][gz].count = 0;
				grid[gx][gy][gz].capacity = 0;
				grid[gx][gy][gz].particles = NULL;
				
				work_grid[gx][gy][gz].count = 0;
				work_grid[gx][gy][gz].capacity = 0;
				work_grid[gx][gy][gz].particles = NULL;
				
				/* Initialize temporary grids as well */
				temp_grid1[gx][gy][gz].count = 0;
				temp_grid1[gx][gy][gz].capacity = 0;
				temp_grid1[gx][gy][gz].particles = NULL;
				
				temp_grid2[gx][gy][gz].count = 0;
				temp_grid2[gx][gy][gz].capacity = 0;
				temp_grid2[gx][gy][gz].particles = NULL;
			}
		}
	}
	
	/* Create particles randomly */
	while (particles_created < total_particles) {
		new_particle.x = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		new_particle.y = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		new_particle.z = ((float)rand() / RAND_MAX) * 2.0f - 1.0f;
		new_particle.vx = 0.0f;
		new_particle.vy = 0.0f;
		new_particle.vz = 0.0f;
		new_particle.type = rand() % NUM_TYPES;
		
		/* Find correct grid cell */
		gx = coord_to_grid(new_particle.x);
		gy = coord_to_grid(new_particle.y);
		gz = coord_to_grid(new_particle.z);
		
		/* Add particle */
		add_particle_to_grid(&grid[gx][gy][gz], new_particle);
		particles_created++;
	}
	
	printf("Created %d particles\n", particles_created);
}

/* Structure for sending data to worker threads */
typedef struct {
	int thread_id;  /* 0 for even cells, 1 for odd cells */
} ThreadData;

/* Worker function to process particles */
static void* particle_worker(void* arg) {
	ThreadData* data = (ThreadData*)arg;
	int thread_id = data->thread_id;
	int gx, gy, gz, ngx, ngy, ngz;
	int i, j, new_gx, new_gy, new_gz;
	float dx, dy, dz, dist_sq, dist, force;
	float fx, fy, fz;
	float vmix, center_force, base_radius, collision_force;
	float radius_i, radius_j, min_dist, collision_start_sq, collision_intensity;
	Cell updated_particle;
	Cell *current_particle, *other_particle;
	float max_dist_sq = 0.06f;  /* Reduced from 0.09f for better performance */
	
	/* Physics parameters - reduced friction for more lively movements */
	vmix = 0.95f;  
	center_force = 0.05f;
	base_radius = 0.02f;
	collision_force = 0.005f;
	
	while (threads_running) {
		/* Wait for all threads to start */
		pthread_barrier_wait(&barrier);
		
		if (!threads_running) break;
		
		/* Clear this thread's temporary grid */
		for (gx = 0; gx < GRID_SIZE; gx++) {
			for (gy = 0; gy < GRID_SIZE; gy++) {
				for (gz = 0; gz < GRID_SIZE; gz++) {
					if (thread_id == 0) {
						temp_grid1[gx][gy][gz].count = 0;
					} else {
						temp_grid2[gx][gy][gz].count = 0;
					}
				}
			}
		}
		
		/* Process grid cells based on thread ID (even/odd cells) */
		for (gx = 0; gx < GRID_SIZE; gx++) {
			for (gy = 0; gy < GRID_SIZE; gy++) {
				for (gz = 0; gz < GRID_SIZE; gz++) {
					/* Divide work: even/odd grid cells */
					int cell_sum = gx + gy + gz;
					if ((cell_sum % 2) != thread_id) continue;
					
					/* Skip empty cells */
					if (grid[gx][gy][gz].count == 0) continue;
					
					/* Process ALL particles in this grid cell */
					for (i = 0; i < grid[gx][gy][gz].count; i++) {
						current_particle = &grid[gx][gy][gz].particles[i];
						fx = fy = fz = 0.0f;
						
						/* Calculate this particle's radius */
						radius_i = base_radius + (current_particle->z + 1.0f) * 0.01f;
						
						/* Central attraction toward origin */
						dx = -current_particle->x;
						dy = -current_particle->y;
						dz = -current_particle->z;
						dist_sq = dx*dx + dy*dy + dz*dz;
						if (dist_sq > 0.0001f) {
							dist = sqrt(dist_sq);
							force = center_force / dist;
							fx += force * dx;
							fy += force * dy;
							fz += force * dz;
						}
						
						/* Check neighboring cells for interactions - optimized loop order */
						for (ngx = gx - 1; ngx <= gx + 1; ngx++) {
							if (ngx < 0 || ngx >= GRID_SIZE) continue;
							for (ngy = gy - 1; ngy <= gy + 1; ngy++) {
								if (ngy < 0 || ngy >= GRID_SIZE) continue;
								for (ngz = gz - 1; ngz <= gz + 1; ngz++) {
									if (ngz < 0 || ngz >= GRID_SIZE) continue;
									
									/* Skip empty cells early */
									if (grid[ngx][ngy][ngz].count == 0) continue;
									
									/* Interact with particles in this neighboring cell */
									for (j = 0; j < grid[ngx][ngy][ngz].count; j++) {
										other_particle = &grid[ngx][ngy][ngz].particles[j];
										
										/* Skip self */
										if (current_particle == other_particle) continue;
										
										/* Fast distance check without wraparound first */
										dx = current_particle->x - other_particle->x;
										dy = current_particle->y - other_particle->y;
										dz = current_particle->z - other_particle->z;
										dist_sq = dx*dx + dy*dy + dz*dz;
										
										/* Early distance cutoff */
										if (dist_sq > max_dist_sq) {
											/* Check if wraparound might help */
											if (dx > 1.0f || dx < -1.0f || 
												dy > 1.0f || dy < -1.0f || 
												dz > 1.0f || dz < -1.0f) {
												/* Only then calculate periodic distance */
												calc_periodic_distance(current_particle->x, current_particle->y, current_particle->z, 
																	   other_particle->x, other_particle->y, other_particle->z, 
																	   &dx, &dy, &dz);
												dist_sq = dx*dx + dy*dy + dz*dz;
												if (dist_sq > max_dist_sq) continue;
											} else {
												continue;
											}
										}
										
										if (dist_sq < 0.0001f) continue;
										
										/* Calculate other particle's radius */
										radius_j = base_radius + (other_particle->z + 1.0f) * 0.01f;
										min_dist = radius_i + radius_j;
										collision_start_sq = min_dist * min_dist * 9.0f;
										
										/* Collision or attraction handling */
										if (dist_sq < collision_start_sq) {
											dist = sqrt(dist_sq);
											collision_intensity = (sqrt(collision_start_sq) - dist) / sqrt(collision_start_sq);
											force = collision_force * collision_intensity / dist_sq;
											fx += force * dx;
											fy += force * dy;
											fz += force * dz;
										} else {
											dist = sqrt(dist_sq);
											force = attraction[current_particle->type][other_particle->type] / dist;
											fx += force * dx;
											fy += force * dy;
											fz += force * dz;
										}
									}
								}
							}
						}
						
						/* Create updated particle */
						updated_particle = *current_particle;
						
						/* Update velocity and position */
						updated_particle.vx = updated_particle.vx * vmix + fx * 0.005f;
						updated_particle.vy = updated_particle.vy * vmix + fy * 0.005f;
						updated_particle.vz = updated_particle.vz * vmix + fz * 0.005f;
						
						updated_particle.x += updated_particle.vx;
						updated_particle.y += updated_particle.vy;
						updated_particle.z += updated_particle.vz;
						
						/* Wrap-around handling */
						if (updated_particle.x > 1.0f) updated_particle.x -= 2.0f;
						else if (updated_particle.x < -1.0f) updated_particle.x += 2.0f;
						
						if (updated_particle.y > 1.0f) updated_particle.y -= 2.0f;
						else if (updated_particle.y < -1.0f) updated_particle.y += 2.0f;
						
						if (updated_particle.z > 1.0f) updated_particle.z -= 2.0f;
						else if (updated_particle.z < -1.0f) updated_particle.z += 2.0f;
						
						/* Find new grid position for the updated particle */
						new_gx = coord_to_grid(updated_particle.x);
						new_gy = coord_to_grid(updated_particle.y);
						new_gz = coord_to_grid(updated_particle.z);
						
						/* Add to this thread's temporary grid (THREAD-SAFE) */
						if (thread_id == 0) {
							add_particle_to_grid(&temp_grid1[new_gx][new_gy][new_gz], updated_particle);
						} else {
							add_particle_to_grid(&temp_grid2[new_gx][new_gy][new_gz], updated_particle);
						}
					}
				}
			}
		}
		
		/* Wait for all threads to be done */
		pthread_barrier_wait(&barrier);
	}
	
	return NULL;
}

void update_particles(void) {
	int gx, gy, gz, i;
	
	/* STEP 1: Clear work_grid */
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				work_grid[gx][gy][gz].count = 0;
			}
		}
	}
	
	/* STEP 2: Signal threads to start working */
	pthread_barrier_wait(&barrier);
	
	/* STEP 3: Wait for threads to finish */
	pthread_barrier_wait(&barrier);
	
	/* STEP 4: Combine results from both temporary grids to work_grid */
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				/* Copy particles from temp_grid1 */
				for (i = 0; i < temp_grid1[gx][gy][gz].count; i++) {
					add_particle_to_grid(&work_grid[gx][gy][gz], 
									   temp_grid1[gx][gy][gz].particles[i]);
				}
				
				/* Copy particles from temp_grid2 */
				for (i = 0; i < temp_grid2[gx][gy][gz].count; i++) {
					add_particle_to_grid(&work_grid[gx][gy][gz], 
									   temp_grid2[gx][gy][gz].particles[i]);
				}
			}
		}
	}
	
	/* STEP 5: Swap grid and work_grid */
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				GridCell temp = grid[gx][gy][gz];
				grid[gx][gy][gz] = work_grid[gx][gy][gz];
				work_grid[gx][gy][gz] = temp;
			}
		}
	}
}

/* Threading functions for pthread implementation */
void init_threads(void) {
	static ThreadData thread_data[2];
	
	/* Initialize barrier for 3 threads (main + 2 worker threads) */
	if (pthread_barrier_init(&barrier, NULL, 3) != 0) {
		printf("CRITICAL ERROR: Could not initialize pthread barrier!\n");
		return;
	}
	
	threads_running = 1;
	
	/* Create thread data */
	thread_data[0].thread_id = 0;  /* Even cells */
	thread_data[1].thread_id = 1;  /* Odd cells */
	
	/* Create worker threads */
	if (pthread_create(&thread1, NULL, particle_worker, &thread_data[0]) != 0) {
		printf("CRITICAL ERROR: Could not create thread 1!\n");
		threads_running = 0;
		return;
	}
	
	if (pthread_create(&thread2, NULL, particle_worker, &thread_data[1]) != 0) {
		printf("CRITICAL ERROR: Could not create thread 2!\n");
		threads_running = 0;
		pthread_cancel(thread1);
		return;
	}
	
	printf("Pthread system initialized with 2 worker threads\n");
}

void cleanup_threads(void) {
	int gx, gy, gz;
	
	/* Stop threads if they are running */
	if (threads_running) {
		threads_running = 0;
		
		/* Signal threads to exit */
		pthread_barrier_wait(&barrier);
		
		/* Wait for threads to finish */
		pthread_join(thread1, NULL);
		pthread_join(thread2, NULL);
		
		/* Destroy barrier */
		pthread_barrier_destroy(&barrier);
		
		printf("Pthread system shut down\n");
	}
	
	/* Free memory from all grids */
	for (gx = 0; gx < GRID_SIZE; gx++) {
		for (gy = 0; gy < GRID_SIZE; gy++) {
			for (gz = 0; gz < GRID_SIZE; gz++) {
				if (grid[gx][gy][gz].particles) {
					free(grid[gx][gy][gz].particles);
					grid[gx][gy][gz].particles = NULL;
				}
				if (work_grid[gx][gy][gz].particles) {
					free(work_grid[gx][gy][gz].particles);
					work_grid[gx][gy][gz].particles = NULL;
				}
				if (temp_grid1[gx][gy][gz].particles) {
					free(temp_grid1[gx][gy][gz].particles);
					temp_grid1[gx][gy][gz].particles = NULL;
				}
				if (temp_grid2[gx][gy][gz].particles) {
					free(temp_grid2[gx][gy][gz].particles);
					temp_grid2[gx][gy][gz].particles = NULL;
				}
			}
		}
	}
}
