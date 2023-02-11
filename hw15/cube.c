#include "SDL2/SDL.h"
#include <GL/gl.h>
#include <GL/glu.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#define SCREEN_WIDTH 480
#define SCREEN_HEIGHT 480

SDL_Window *gWindow = NULL;
SDL_GLContext *gContext = NULL;
bool gRenderQuad = true;
float angle = 0;

bool initGL() {
  GLenum error = GL_NO_ERROR;
  glMatrixMode(GL_PROJECTION);
  glLoadIdentity();
  error = glGetError();
  if (error != GL_NO_ERROR) {
    printf("Error initializing OpenGL! %s\n", gluErrorString(error));
    return false;
  }
  // Initialize clear color
  glClearColor(0.f, 0.f, 0.f, 1.f);
  return true;
}


bool init() {
  if (SDL_Init(SDL_INIT_VIDEO) < 0) {
    printf("SDL could not initialize! SDL Error: %s\n", SDL_GetError());
    return false;
  }

  gWindow = SDL_CreateWindow("Мой куб SDL + OpenGL", SDL_WINDOWPOS_UNDEFINED, SDL_WINDOWPOS_UNDEFINED,
                             SCREEN_WIDTH, SCREEN_HEIGHT, SDL_WINDOW_OPENGL | SDL_WINDOW_SHOWN);
  if (gWindow == NULL) {
    printf("Window could not be created! SDL Error: %s\n", SDL_GetError());
    return false;
  }

  gContext = SDL_GL_CreateContext(gWindow);
  if (gContext == NULL) {
    printf("OpenGL context could not be created! SDL Error: %s\n", SDL_GetError());
    return false;
  }

  if (SDL_GL_SetSwapInterval(1) < 0) {
    printf("Warning: Unable to set VSync! SDL Error: %s\n", SDL_GetError());
  }

  if (!initGL()) {
    printf("Unable to initialize OpenGL!\n");
    return false;
  }
  return true;
}


void render() {
  glLoadIdentity();
  glRotatef(angle, 0.f, 1.0f, 1.f);
  glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
  if (gRenderQuad) {
    glBegin(GL_QUADS);
    glColor3f(0.0f, 1.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);

    glColor3f(1.0f, 0.0f, 0.0f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);

    glColor3f(1.0f, 1.0f, 1.0f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);

    glColor3f(1.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, -0.5f);

    glColor3f(1.0f, 1.0f, 0.0f);
    glVertex3f(0.5f, -0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, -0.5f);
    glVertex3f(0.5f, 0.5f, 0.5f);
    glVertex3f(0.5f, -0.5f, 0.5f);

    glColor3f(0.0f, 0.0f, 1.0f);
    glVertex3f(-0.5f, -0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, -0.5f);
    glVertex3f(-0.5f, 0.5f, 0.5f);
    glVertex3f(-0.5f, -0.5f, 0.5f);
    glEnd();
    glPopMatrix();
    glEnable(GL_DEPTH_TEST);
  }
}


static void release() {
  if (gContext) {
    SDL_GL_DeleteContext(gContext);
    gContext = NULL;
  }
  if (gWindow) {
    SDL_DestroyWindow(gWindow);
    gWindow = NULL;
  }
  SDL_Quit();
}


int main() {
  // Start up SDL and create window
  if (!init()) {
    printf("Failed to initialize!\n");
    return EXIT_FAILURE;
  }

  bool quit = false;
  SDL_Event event;
  SDL_StartTextInput();

  while (!quit) {
    // Handle events on queue
    while (SDL_PollEvent(&event) != 0) {
      // User requests quit
      if (event.type == SDL_QUIT) {
        quit = true;
      } else if (event.type == SDL_KEYDOWN && event.key.keysym.sym == SDLK_ESCAPE) {
        quit = true;
      }
    }
    angle = angle + 1 % 360;
    usleep(10000); // 10 мс.
    render();
    SDL_GL_SwapWindow(gWindow);
  }
  SDL_StopTextInput();

  release(); // Free resources and close SDL
  return EXIT_SUCCESS;
}
