/*!\file window.c 
 * \brief Utilisation du raster DIY comme pipeline de rendu 3D. Cet
 * exemple montre l'affichage d'une grille de cubes.
 * \author Farès BELHADJ, amsi@up8.edu
 * \date December 02, 2021.
 */
#include <assert.h>
/* inclusion des entêtes de fonctions de gestion de primitives simples
 * de dessin. La lettre p signifie aussi bien primitive que
 * pédagogique. */
#include <GL4D/gl4dp.h>
/* inclure la bibliothèque de rendu DIY */
#include "rasterize.h"

/* inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>

/* protos de fonctions locales (static) */
static void init(void);
static void draw(void);
static void key(int keycode);
static void sortie(void);

/*!\brief une surface représentant un cube */
static surface_t * _cube = NULL;

/* des variable d'états pour activer/désactiver des options de rendu */
static int _use_tex = 1, _use_color = 1, _use_lighting = 1;


/* on créé une grille de positions où il y aura des cubes */
static int _grille[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 0, 2, 0, 2, 0, 2, 0, 2, 0, 2, 0, 1,
  1, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
};
static int _grilleW = 13;
static int _grilleH = 13;

/*!\brief paramètre l'application et lance la boucle infinie. */
int main(int argc, char ** argv) {
  /* tentative de création d'une fenêtre pour GL4Dummies */
  if(!gl4duwCreateWindow(argc, argv, /* args du programme */
			 "The DIY Rasterizer", /* titre */
			 10, 10, 800, 600, /* x, y, largeur, heuteur */
			 GL4DW_SHOWN) /* état visible */) {
    /* ici si échec de la création souvent lié à un problème d'absence
     * de contexte graphique ou d'impossibilité d'ouverture d'un
     * contexte OpenGL (au moins 3.2) */
    return 1;
  }
  init();
  /* mettre en place la fonction d'interception clavier */
  gl4duwKeyDownFunc(key);
  /* mettre en place la fonction de display */
  gl4duwDisplayFunc(draw);
  /* boucle infinie pour éviter que le programme ne s'arrête et ferme
   * la fenêtre immédiatement */
  gl4duwMainLoop();
  return 0;
}

/*!\brief init de nos données, spécialement les trois surfaces
 * utilisées dans ce code */
void init(void) {
  GLuint id;
  vec4 r = {1, 0, 0, 1}, g = {0, 1, 0, 1}, b = {0, 0, 1, 1};
  /* création d'un screen GL4Dummies (texture dans laquelle nous
   * pouvons dessiner) aux dimensions de la fenêtre.  IMPORTANT de
   * créer le screen avant d'utiliser les fonctions liées au
   * textures */
  gl4dpInitScreen();
  /* Pour forcer la désactivation de la synchronisation verticale */
  SDL_GL_SetSwapInterval(0);
  /* on créé le cube */
  _cube   =   mk_cube();         /* ça fait 2x6 triangles      */
  /* on change la couleur */
  _cube->dcolor = b; 
  /* on leur rajoute la texture */
  id = get_texture_from_BMP("images/tex.bmp");
  set_texture_id(  _cube, id);
  /* si _use_tex != 0, on active l'utilisation de la texture */
  if(_use_tex) {
    enable_surface_option(  _cube, SO_USE_TEXTURE);
  }
  /* si _use_lighting != 0, on active l'ombrage */
  if(_use_lighting) {
    enable_surface_option(  _cube, SO_USE_LIGHTING);
  }
  /* mettre en place la fonction à appeler en cas de sortie */
  atexit(sortie);
}

/*!\brief la fonction appelée à chaque display. */
void draw(void) {
  /* on va récupérer le delta-temps */
  static double t0 = 0.0; // le temps à la frame précédente
  double t, dt;
  t = gl4dGetElapsedTime();
  dt = (t - t0) / 1000.0; // diviser par mille pour avoir des secondes
  // pour le frame d'après, je mets à jour t0
  t0 = t;
  /* fin de récupération de delta-temps */
  static float a = 0.0f;
  float model_view_matrix[16], projection_matrix[16], nmv[16];
  /* effacer l'écran et le buffer de profondeur */
  gl4dpClearScreen();
  clear_depth_map();
  /* des macros facilitant le travail avec des matrices et des
   * vecteurs se trouvent dans la bibliothèque GL4Dummies, dans le
   * fichier gl4dm.h */
  /* charger un frustum dans projection_matrix */
  MFRUSTUM(projection_matrix, -0.05f, 0.05f, -0.05f, 0.05f, 0.1f, 1000.0f);
  /* charger la matrice identité dans model-view */
  MIDENTITY(model_view_matrix);
  /* on place la caméra en arrière-haut, elle regarde le centre de la scène */
  lookAt(model_view_matrix, 0, 50, 50, 0, 0, 0, 0, 0, -1);

  /* pour centrer la grille par rapport au monde */
  float cX = -2.0f * _grilleW / 2.0f;
  float cZ = -2.0f * _grilleH / 2.0f;
  /* pour toutes les cases de la grille, afficher un cube quand il y a
   * un 1 dans la grille */
  for(int i = 0; i < _grilleW; ++i) {
    for(int j = 0; j < _grilleH; ++j) {
      if(_grille[i * _grilleW + j] == 2){
        memcpy(nmv, model_view_matrix, sizeof nmv);
        translate(nmv, 2.0f * j + cX, 0.0f, 2.0f * i + cZ);
        transform_n_rasterize(_cube, nmv, projection_matrix);
      }
      if(_grille[i * _grilleW + j] == 1) {
	      /* copie model_view_matrix dans nmv */
        memcpy(nmv, model_view_matrix, sizeof nmv);
        /* pour tourner tout le plateau */
        //rotate(nmv, a, 0.0f, 1.0f, 0.0f);
        /* pour convertir les coordonnées i,j de la grille en x,z du monde */
        translate(nmv, 2.0f * j + cX, 0.0f, 2.0f * i + cZ);
        transform_n_rasterize(_cube, nmv, projection_matrix);
      }
    }
  }


  /* déclarer qu'on a changé des pixels du screen (en bas niveau) */
  gl4dpScreenHasChanged();
  /* fonction permettant de raffraîchir l'ensemble de la fenêtre*/
  gl4dpUpdateScreen(NULL);
  a += 0.1f * 360.0f * dt;
}

/*!\brief intercepte l'événement clavier pour modifier les options. */
void key(int keycode) {
  switch(keycode) {
  case GL4DK_t: /* 't' la texture */
    _use_tex = !_use_tex;
    if(_use_tex) {
      enable_surface_option(  _cube, SO_USE_TEXTURE);
    } else {
      disable_surface_option(  _cube, SO_USE_TEXTURE);
    }
    break;
  case GL4DK_c: /* 'c' utiliser la couleur */
    _use_color = !_use_color;
    if(_use_color) {
      enable_surface_option(  _cube, SO_USE_COLOR);
    } else { 
      disable_surface_option(  _cube, SO_USE_COLOR);
    }
    break;
  case GL4DK_l: /* 'l' utiliser l'ombrage par la méthode Gouraud */
    _use_lighting = !_use_lighting;
    if(_use_lighting) {
      enable_surface_option(  _cube, SO_USE_LIGHTING);
    } else { 
      disable_surface_option(  _cube, SO_USE_LIGHTING);
    }
    break;
  default: break;
  }
}

/*!\brief à appeler à la sortie du programme. */
void sortie(void) {
  /* on libère le cube */
  if(_cube) {
    free_surface(_cube);
    _cube = NULL;
  }
  /* libère tous les objets produits par GL4Dummies, ici
   * principalement les screen */
  gl4duClean(GL4DU_ALL);
}