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
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
/* inclusion des entêtes de fonctions de création et de gestion de
 * fenêtres système ouvrant un contexte favorable à GL4dummies. Cette
 * partie est dépendante de la bibliothèque SDL2 */
#include <GL4D/gl4duw_SDL2.h>

/* protos de fonctions locales (static) */
static void init(void);
static void draw(void);
static void keyd(int keycode);
static void idle(void);
static void keyu(int keycode);
static void sortie(void); 

/*!\brief une surface représentant un cube */
static surface_t * _cube = NULL;
static surface_t * _cubeM = NULL;
static surface_t * _sphere = NULL;
static surface_t * _bombe = NULL;
/* des variable d'états pour activer/désactiver des options de rendu */
static int _use_tex = 1, _use_color = 1, _use_lighting = 1;
typedef struct perso{
  int pos_x;
  int pos_y;
}Perso;
Perso perso;

typedef struct bombe{
  int position;
  int avant_explosion;
  struct bombe* next;
}Bombe;

typedef Bombe* Bombel;
static Bombel tab = NULL; 

enum {
  VK_RIGHT = 0,
  VK_UP,
  VK_LEFT,
  VK_DOWN,
  VK_SPACE,
  /* toujours à la fin */
  VK_SIZEOF
};
int _vkeyboard[VK_SIZEOF] = {0, 0, 0, 0, 0};
//fonction a faire remplir_tab(bombe* tab, int position), incrementer_all(bombe* tab), detruire_bombe(bombe* tab) Bombe* crcer_bombe(int position)
Bombe* creer_bombe(int position){
  Bombe* tab= NULL;
  tab = malloc(sizeof(Bombe));
  if (tab == NULL) { 
    exit(1);
  }
  tab->next= NULL;
  tab->avant_explosion = 0;
  tab->position = position;
  return tab;
}
void remplir_tab(Bombel* tab, int position){
  if(*tab == NULL){
    *tab = creer_bombe(position);
    return;
  }
  else{
    Bombe* l = creer_bombe(position);
    l->next = *tab;
    *tab = l;
    return;
  }
}
void incrementer_all(Bombel* tab){
  Bombel tmp=*tab;
  while (*tab == NULL) {
    (*tab)->avant_explosion += 1;
    *tab = (*tab)->next;
  }
  *tab= tmp;
}
/* on créé une grille de positions où il y aura des cubes */
static int _grille[] = {
  1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
  1, 3, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1,
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

void detruire_bombe(Bombel* tab){
  Bombel tmp=*tab;
  Bombel last=NULL;
  while (*tab != NULL) {
    if ((*tab)->avant_explosion == 3) {
      _grille[(*tab)->position] = 0;
      if (last ==NULL) {
        last = *tab;
        *tab = (*tab)->next;
        tmp=*tab;
      }
      else if ((*tab)->next == NULL) {
        *tab = NULL;
      }
      else{
        *tab = (*tab)->next;
        last->next = *tab;
      }
    }
    else{
      *tab = (*tab)->next;
    }
  }
  *tab= tmp;
}

static int _grilleW = 13;
static int _grilleH = 13;

static int last;
static int ready = 0;
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
  gl4duwKeyDownFunc(keyd);
  /* mettre en place la fonction d'interception clavier touche relachée */
  gl4duwKeyUpFunc(keyu);
  /* mettre en place la fonction idle (simulation, au sens physique du terme) */
  gl4duwIdleFunc(idle);
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
  vec4 r = {1, 0, 0, 1}, g = {0, 1, 0, 1}, b = {0, 0, 1, 1}, y = {1, 0, 1, 1};
  /* création d'un screen GL4Dummies (texture dans laquelle nous
   * pouvons dessiner) aux dimensions de la fenêtre.  IMPORTANT de
   * créer le screen avant d'utiliser les fonctions liées au
   * textures */
  gl4dpInitScreen();
  SDL_GL_SetSwapInterval(1);
  /* Pour forcer la désactivation de la synchronisation verticale */
  SDL_GL_SetSwapInterval(0);
  /* on créé le cube */
  _cube   =   mk_cube();         /* ça fait 2x6 triangles      */
  _cubeM =   mk_cube(); 
  _sphere   =   mk_sphere(12,12);
  _bombe = mk_sphere(12,12); // Partie bombe a améliorer
  /* on change la couleur */
  _cube->dcolor = b;  
  _sphere->dcolor = r;
  _bombe->dcolor = y;
  _cubeM->dcolor = g;
  /* on leur rajoute la texture */
  id = get_texture_from_BMP("images/tex.bmp");
  set_texture_id(  _cube, id);
  set_texture_id(  _cubeM, id);
  set_texture_id(  _sphere, id);
  set_texture_id(  _bombe, id);
  /* si _use_tex != 0, on active l'utilisation de la texture */
  if(_use_tex) {
    enable_surface_option(  _cube, SO_USE_TEXTURE);
    enable_surface_option(  _cubeM, SO_USE_TEXTURE);
    enable_surface_option(  _sphere, SO_USE_TEXTURE);
    enable_surface_option(  _bombe, SO_USE_TEXTURE);
  }
  /* si _use_lighting != 0, on active l'ombrage */
  if(_use_lighting) {
    enable_surface_option(  _cube, SO_USE_LIGHTING);
    enable_surface_option(  _cubeM, SO_USE_LIGHTING);
    enable_surface_option(  _sphere, SO_USE_LIGHTING);
    enable_surface_option(  _bombe, SO_USE_LIGHTING);
  }
  /* mettre en place la fonction à appeler en cas de sortie */
  atexit(sortie);
}

void idle(void) {
  /* on va récupérer le delta-temps */
  
  if(_vkeyboard[VK_RIGHT]){
    if( _grille[ perso.pos_x * _grilleW +  perso.pos_y + 1 ] != 0 ){
      return;
    }
    else { 
      _grille[ perso.pos_x * _grilleW +  perso.pos_y]=0;
      _grille[ perso.pos_x * _grilleW +  perso.pos_y + 1]=3;
      last = _vkeyboard[VK_RIGHT];
      _vkeyboard[VK_RIGHT] = 0;
      return;
    }
  }
  if(_vkeyboard[VK_UP]==2){
    if( _grille[ perso.pos_x * _grilleW +  perso.pos_y - 13 ] != 0 ){
      return;
    }
    else {
      _grille[ perso.pos_x * _grilleW +  perso.pos_y]=0;
      _grille[ perso.pos_x * _grilleW +  perso.pos_y -13]=3;
      last = _vkeyboard[VK_UP];
      _vkeyboard[VK_UP] = 0;
      return;
    }
  }
  if(_vkeyboard[VK_LEFT]==3){
    if( _grille[ perso.pos_x * _grilleW +  perso.pos_y - 1 ] != 0 ){
      return;
    }
    else { 
      _grille[ perso.pos_x * _grilleW +  perso.pos_y]=0;
      _grille[ perso.pos_x * _grilleW +  perso.pos_y - 1]=3;
      last = _vkeyboard[VK_LEFT];
      _vkeyboard[VK_LEFT]=0;
      return;
    }
  }
  if(_vkeyboard[VK_DOWN]==4){
    if( _grille[ perso.pos_x * _grilleW +  perso.pos_y +13 ] != 0 ){
        return;
    }
    else { 
      _grille[ perso.pos_x * _grilleW +  perso.pos_y]=0;
      _grille[ perso.pos_x * _grilleW +  perso.pos_y +13]=3;
      last = _vkeyboard[VK_DOWN];
      _vkeyboard[VK_DOWN] = 0;
      return;
    }
  }
  if(_vkeyboard[VK_SPACE]==5){
    if(last == 1){
      if( _grille[ perso.pos_x * _grilleW +  perso.pos_y + 1 ] != 0 ){
        return;
      }
      else { 
        _grille[ perso.pos_x * _grilleW +  perso.pos_y + 1]=4;
        return;
      }
    }
    else if(last==2){
      if( _grille[ perso.pos_x * _grilleW +  perso.pos_y - 13 ] != 0 ){
        return;
      }
      else {
        _grille[ perso.pos_x * _grilleW +  perso.pos_y -13]=4;
        return;
      }
    }
    else if(last==3){
      if( _grille[ perso.pos_x * _grilleW +  perso.pos_y - 1 ] != 0 ){
        return;
      }
      else { 
        _grille[ perso.pos_x * _grilleW +  perso.pos_y - 1]=4;
        return;
      }
    }
    else if(last==4){
      if( _grille[ perso.pos_x * _grilleW +  perso.pos_y +13 ] != 0 ){
          return;
      }
      else { 
        _grille[ perso.pos_x * _grilleW +  perso.pos_y +13]=4;
        return;
      }
    }
    else{
      return;
    }
  }

}

// void Lrand(int nb_cube){
//   static int i =0;
//   if(i == 0){
//       srand( time( NULL ) );
//     float model_view_matrix[16], projection_matrix[16], nmv[16];
//     /* effacer l'écran et le buffer de profondeur */
//     gl4dpClearScreen();
//     clear_depth_map();
//     /* des macros facilitant le travail avec des matrices et des
//      vecteurs se trouvent dans la bibliothèque GL4Dummies, dans le
//      fichier gl4dm.h */
//     /* charger un frustum dans projection_matrix */
//     MFRUSTUM(projection_matrix, -0.05f, 0.05f, -0.05f, 0.05f, 0.1f, 1000.0f);
//     /* charger la matrice identité dans model-view */
//     MIDENTITY(model_view_matrix);
//     while (nb_cube != 0 ) {
//       int searchedValue = rand() % 169;incrementer_all(&tab);
//        detruire_bombe(&tab);
//       if (_grille[searchedValue] == 0) {
//         _grille[searchedValue] = 1;
//         nb_cube -=1;
//       }
//     }
//     i = 1;
//   }
// }



/*!\brief la fonction appelée à chaque display. */
void draw(void) {
  // le temps à la frame précédente
  int t, dt;
  t = gl4dGetElapsedTime();
  dt = t  / 1000.0; // diviser par mille pour avoir des secondes
  // pour le frame d'après, je mets à jour t0
  if (dt == 3) {
    printf("%d\n",dt); 
  }
  vec4 r = {1, 0, 0, 1}, y = {1, 0, 1, 1};
  /* on va récupérer le delta-temps */
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
  //Lrand(25);
  for(int i = 0; i < _grilleW; ++i) { // place les cubes a l'emplecement de la grille 
    for(int j = 0; j < _grilleH; ++j) {
      if(_grille[i * _grilleW + j] == 4){
        memcpy(nmv, model_view_matrix, sizeof nmv);
        translate(nmv, 2.0f * j + cX, 0.0f, 2.0f * i + cZ);
        transform_n_rasterize(_bombe, nmv, projection_matrix);
        if ( ready == 0) {
          ready = 1;
          _bombe->dcolor = r;
        }
        else{
          ready = 0;
          _bombe->dcolor = y;
        }
        SDL_Delay(100);
      }
      if(_grille[i * _grilleW + j] == 3){
        memcpy(nmv, model_view_matrix, sizeof nmv);
        translate(nmv, 2.0f * j + cX, 0.0f, 2.0f * i + cZ);
        transform_n_rasterize(_cube, nmv, projection_matrix);
        translate(nmv, 0.0f, 2.0f, 0.0f);
        transform_n_rasterize(_sphere, nmv, projection_matrix);
        perso.pos_x = i;
        perso.pos_y = j;
      }
      if(_grille[i * _grilleW + j] == 2){
        memcpy(nmv, model_view_matrix, sizeof nmv);
        translate(nmv, 2.0f * j + cX, 0.0f, 2.0f * i + cZ);
        transform_n_rasterize(_cubeM, nmv, projection_matrix);
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
}

/*!\brief intercepte l'événement clavier pour modifier les options. */
void keyd(int keycode) {
  //_grille[ perso.pos_x * _grilleW +  perso.pos_y +13 ]=4;
  switch(keycode) {
  case GL4DK_t: /* 't' la texture */
    _use_tex = !_use_tex;
    if(_use_tex) {
      enable_surface_option(  _cube, SO_USE_TEXTURE);
      enable_surface_option(  _cubeM, SO_USE_TEXTURE);
      enable_surface_option(  _sphere, SO_USE_TEXTURE);
      enable_surface_option(  _bombe, SO_USE_TEXTURE);
    } else {
      disable_surface_option(  _cube, SO_USE_TEXTURE);
      disable_surface_option(  _cubeM, SO_USE_TEXTURE);
      disable_surface_option(  _sphere, SO_USE_TEXTURE);
      disable_surface_option(  _bombe, SO_USE_TEXTURE);
    }
    break;
  case GL4DK_c: /* 'c' utiliser la couleur */
    _use_color = !_use_color;
    if(_use_color) {
      enable_surface_option(  _cube, SO_USE_COLOR);
      enable_surface_option(  _cubeM, SO_USE_COLOR);
      enable_surface_option(  _sphere, SO_USE_COLOR);
      enable_surface_option(  _bombe, SO_USE_COLOR);
    } else { 
      disable_surface_option(  _cube, SO_USE_COLOR);
      disable_surface_option(  _cubeM, SO_USE_COLOR);
      disable_surface_option(  _sphere, SO_USE_COLOR);
      disable_surface_option(  _bombe, SO_USE_COLOR);
    }
    break;
  case GL4DK_l: /* 'l' utiliser l'ombrage par la méthode Gouraud */
    _use_lighting = !_use_lighting;
    if(_use_lighting) {
      enable_surface_option(  _cube, SO_USE_LIGHTING);
      enable_surface_option(  _cubeM, SO_USE_LIGHTING);
      enable_surface_option(  _sphere, SO_USE_LIGHTING);
      enable_surface_option(  _bombe, SO_USE_LIGHTING);
    } else { 
      disable_surface_option(  _cube, SO_USE_LIGHTING);
      disable_surface_option(  _cubeM, SO_USE_LIGHTING);
      disable_surface_option(  _sphere, SO_USE_LIGHTING);
      disable_surface_option(  _bombe, SO_USE_LIGHTING);
    }
    case GL4DK_RIGHT:
      _vkeyboard[VK_RIGHT] = 1;
      break;
    case GL4DK_UP:
      _vkeyboard[VK_UP] = 2;
      break;
    case GL4DK_LEFT:
      _vkeyboard[VK_LEFT] = 3;
      break;
    case GL4DK_DOWN:
      _vkeyboard[VK_DOWN] = 4;
      break;
    case GL4DK_SPACE:
      _vkeyboard[VK_SPACE] = 5;
      break;
  default: break;
  }
}

void keyu(int keycode) {
  switch(keycode) {
  case GL4DK_RIGHT:
    _vkeyboard[VK_RIGHT] = 0;
    break;
  case GL4DK_UP:
    _vkeyboard[VK_UP] = 0;
    break;
  case GL4DK_LEFT:
    _vkeyboard[VK_LEFT] = 0;
    break;
  case GL4DK_DOWN:
    _vkeyboard[VK_DOWN] = 0;
    break;
  case GL4DK_SPACE:
    _vkeyboard[VK_SPACE] = 0;
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
  if(_cubeM) {
    free_surface(_cube);
    _cube = NULL;
  }
  if(_sphere){
    free_surface(_sphere);
    _sphere = NULL;
  }
  if(_bombe){
    free_surface(_bombe);
    _bombe = NULL;
  }
  /* libère tous les objets produits par GL4Dummies, ici
   * principalement les screen */
  gl4duClean(GL4DU_ALL);
}
