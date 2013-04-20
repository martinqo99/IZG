/******************************************************************************
 * Projekt - Zaklady pocitacove grafiky - IZG
 * spanel@fit.vutbr.cz
 *
 * $Id: student.c 265 2013-02-20 16:44:40Z spanel $
 */

#include "student.h"
#include "transform.h"
#include "curve.h"

#include <memory.h>
#include <math.h>


/******************************************************************************
 * Globalni promenne a konstanty
 */

/* krivka predstavujici drahu telesa */
S_Curve   * trajectory        = NULL;

/* pocet kroku pri vykreslovani krivky */
const int           CURVE_QUALITY   = 100;

/* dalsi globalni promenne a konstanty */
S_Coords c1;
double t1 = 0.0;


/*****************************************************************************
 * Funkce vytvori vas renderer a nainicializuje jej */

S_Renderer * studrenCreate()
{
    S_StudentRenderer * renderer = (S_StudentRenderer *)malloc(sizeof(S_StudentRenderer));
    IZG_CHECK(renderer, "Cannot allocate enough memory");

    /* inicializace default rendereru */
    renInit(&renderer->base);

    /* nastaveni ukazatelu na vase upravene funkce */
    renderer->base.projectLineFunc = studrenProjectLine;

    /* inicializace pripadnych nove pridanych casti */
    /* ??? */

    return (S_Renderer *)renderer;
}

/****************************************************************************
 * Orezani usecky algoritmem Liang-Barsky pred vykreslenim do frame bufferu
 * Vraci -1 je-li usecka zcela mimo okno
 * x1, y1, x2, y2 - puvodni a pozdeji orezane koncove body usecky
 * u1, u2 - hodnoty parametru u pro orezanou cast usecky <0, 1> */

int studrenCullLine(S_Renderer *pRenderer,
                    int *x1, int *y1, int *x2, int *y2,
                    double *u1, double *u2
                    )
{
  int     dx, dy, p[4], q[4], i;
  float   t1, t2, ri;

  IZG_ASSERT(x1 && y1 && x2 && y2);

  /* vypocet parametru dx, dy */
  dx = *x2 - *x1;
  dy = *y2 - *y1;

  /* vypocet parametru pi a qi */
  p[0] = -dx; q[0] = *x1 - 1;
  p[1] =  dx; q[1] = pRenderer->frame_w - 2 - *x1;
  p[2] = -dy; q[2] = *y1 - 1;
  p[3] =  dy; q[3] = pRenderer->frame_h - 2 - *y1;

  /* nastaveni parametru ui na pocatecni hodnotu */
  *u1 = 0.0f;
  *u2 = 1.0f;

  /* postupne proverime pi a qi */
  for (i = 0; i < 4; ++i) {
    /* vypocet parametru ri */
    ri = 0.0f;
    if (p[i] != 0.0f) {
      ri = (float) q[i] / (float) p[i];
    }

    /* primka smeruje do okna */
    if (p[i] < 0) {
      if (ri > *u2)
        return -1;
      else if (ri > *u1)
        *u1 = ri;
    }
    else if ( p[i] > 0 ) {    /* primka smeruje z okna ven */
      if (ri < *u1)
        return -1;
      else if (ri < *u2)
        *u2 = ri;
    }
    else if ( q[i] < 0 )    /* primka je mimo okno */
      return -1;
  }
  /* spocteme vysledne souradnice x1,y1 a x2,y2 primky */
  if (*u2 < 1) {
    t1 = *u2 * dx;
    t2 = *u2 * dy;
    *x2 = *x1 + ROUND(t1);
    *y2 = *y1 + ROUND(t2);
  }
  if (*u1 > 0) {
    t1 = *u1 * dx;
    t2 = *u1 * dy;
    *x1 = *x1 + ROUND(t1);
    *y1 = *y1 + ROUND(t2);
  }

  return 0;
}

/****************************************************************************
 * Rasterizace usecky do frame bufferu s interpolaci z-souradnice
 * a zapisem do z-bufferu
 * v1, v2 - ukazatele na kocove body usecky ve 3D prostoru pred projekci
 * x1, y1, x2, y2 - souradnice koncovych bodu ve 2D po projekci
 * color - kreslici barva */

void studrenDrawLine(S_Renderer *pRenderer,
                     S_Coords *v1, S_Coords *v2,
                     int x1, int y1,
                     int x2, int y2,
                     S_RGBA color
                     )
{
  int dx, dy, dz, xy_swap, step_y, k1, k2, p, x, y;
  double u1, u2;

  /* orezani do okna */
  if( studrenCullLine(pRenderer, &x1, &y1, &x2, &y2, &u1, &u2) == -1 ) {
    return;
  }

  /* delta v ose x a y */
  dx = ABS(x2 - x1);
  dy = ABS(y2 - y1);

  /* zamena osy x a y v algoritmu vykreslovani */
  xy_swap = 0;
  if(dy > dx) {
    xy_swap = 1;
    SWAP(x1, y1);
    SWAP(x2, y2);
    SWAP(dx, dy);
  }

  /* pripadne prohozeni bodu v ose x */
  if(x1 > x2) {
    SWAP(x1, x2);
    SWAP(y1, y2);
  }

  int tmp = dx == 0 ? 1 : dx;
  dz = ABS(v2->z - v1->z) / tmp;

  /* smer postupu v ose y */
  if(y1 > y2)
    step_y = -1;
  else step_y = 1;

  /* konstanty pro Bresenhamuv algoritmus */
  k1 = 2 * dy;
  k2 = 2 * (dy - dx);
  p  = k1 - dx;

  /* pocatecni prirazeni hodnot */
  /* cyklus vykreslovani usecky */
  for (x = x1, y = y1; x <= x2; ++x) {
    int z = (x - x1)*dz + v1->z;
    if(xy_swap && z <= DEPTH(pRenderer, y, x)) {
      DEPTH(pRenderer, y, x) = z;
      PIXEL(pRenderer, y, x) = color;
    }
    else if (z <= DEPTH(pRenderer, x, y)) {
      DEPTH(pRenderer, x, y) = z;
      PIXEL(pRenderer, x, y) = color;
    }

    /* uprava prediktoru */
    if( p < 0 ) {
      p += k1;
    }
    else {
      p += k2;
      y += step_y;
    }
  }
}


/******************************************************************************
 * Vykresli caru zadanou barvou
 * Pred vykreslenim aplikuje na koncove body cary aktualne nastavene
 * transformacni matice!
 * p1, p2 - koncove body
 * color - kreslici barva */

void studrenProjectLine(S_Renderer *pRenderer, S_Coords *p1, S_Coords *p2, S_RGBA color)
{
  S_Coords    aa, bb;             /* souradnice vrcholu po transformaci ve 3D pred projekci */
  int         u1, v1, u2, v2;     /* souradnice vrcholu po projekci do roviny obrazovky */

  IZG_ASSERT(pRenderer && p1 && p2);

  /* transformace vrcholu matici model */
  trTransformVertex(&aa, p1);
  trTransformVertex(&bb, p2);

  /* promitneme vrcholy na obrazovku */
  trProjectVertex(&u1, &v1, &aa);
  trProjectVertex(&u2, &v2, &bb);

  /* rasterizace usecky */
  studrenDrawLine(pRenderer, &aa, &bb, u1, v1, u2, v2, color);
}

/******************************************************************************
 * Funkce pro vykresleni krivky jako "lomene cary"
 * color - kreslici barva
 * n - pocet bodu na krivce, kolik se ma pouzit pro aproximaci */

void renderCurve(S_Renderer *pRenderer, S_Curve *pCurve, S_RGBA color, int n)
{
  S_Coords c1, c2;
  double step = 1.0/n, t = step;
  for (c1 = curvePoint(pCurve, 0.0); t < 1; t += step, c1 = c2) {
    c2 = curvePoint(pCurve, t);
    studrenProjectLine(pRenderer, &c1, &c2, color);
  }
}

/******************************************************************************
 * Callback funkce volana pri startu aplikace */

void onInit()
{
  /* vytvoreni a inicializace krivky */
  trajectory = curveCreate();
  curveInit(trajectory, 7, 2);

  cvecGet(trajectory->points, 0) = makeCoords(5, 0, 0);
  cvecGet(trajectory->points, 1) = makeCoords(5, 0, -5);
  cvecGet(trajectory->points, 2) = makeCoords(-5, 0, -5);
  cvecGet(trajectory->points, 3) = makeCoords(-5, 0, 0);
  cvecGet(trajectory->points, 4) = makeCoords(-5, 0, 5);
  cvecGet(trajectory->points, 5) = makeCoords(5, 0, 5);
  cvecGet(trajectory->points, 6) = makeCoords(5, 0, 0);

  dvecGet(trajectory->knots, 0) = 0.0;
  dvecGet(trajectory->knots, 1) = 0.0;
  dvecGet(trajectory->knots, 2) = 0.0;
  dvecGet(trajectory->knots, 3) = 0.25;
  dvecGet(trajectory->knots, 4) = 0.5;
  dvecGet(trajectory->knots, 5) = 0.5;
  dvecGet(trajectory->knots, 6) = 0.75;
  dvecGet(trajectory->knots, 7) = 1.0;
  dvecGet(trajectory->knots, 8) = 1.0;
  dvecGet(trajectory->knots, 9) = 1.0;

  dvecGet(trajectory->weights, 0) = 1.0;
  dvecGet(trajectory->weights, 1) = 0.5;
  dvecGet(trajectory->weights, 2) = 0.5;
  dvecGet(trajectory->weights, 3) = 1.0;
  dvecGet(trajectory->weights, 4) = 0.5;
  dvecGet(trajectory->weights, 5) = 0.5;
  dvecGet(trajectory->weights, 6) = 1.0;
}

/******************************************************************************
 * Callback funkce volana pri tiknuti casovace */

void onTimer(int ticks)
{
  /* uprava pozice planetky */
  c1 = curvePoint(trajectory, t1);
  t1 += 0.01;
  if (t1 > 0.98)
    t1 = 0;
}

/******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte pro kresleni dynamicke sceny, ve ktere se "planetky"
 * pohybuji po krivce, tj. mirne elipticke draze. */

void renderScene(S_Renderer *pRenderer, S_Model *pModel)
{
  /* test existence frame bufferu a modelu */
  IZG_ASSERT(pModel && pRenderer);

  /* nastavit projekcni matici */
  trProjectionPerspective(pRenderer->camera_dist, pRenderer->frame_w, pRenderer->frame_h);

  /* vycistit model matici */
  trLoadIdentity();

  /* nejprve nastavime posuv cele sceny od/ke kamere */
  trTranslate(0.0, 0.0, pRenderer->scene_move_z);

  /* pridame posuv cele sceny v rovine XY */
  trTranslate(pRenderer->scene_move_x, pRenderer->scene_move_y, 0.0);

  /* natoceni cele sceny - jen ve dvou smerech - mys je jen 2D... :( */
  trRotateX(pRenderer->scene_rot_x);
  trRotateY(pRenderer->scene_rot_y);

  /* nastavime material */
  renMatAmbient(pRenderer, &MAT_RED_AMBIENT);
  renMatDiffuse(pRenderer, &MAT_RED_DIFFUSE);
  renMatSpecular(pRenderer, &MAT_RED_SPECULAR);

  const S_RGBA COLOR_GREEN = {0, 255, 0, 255};

  const S_Material COLOR_CYAN_AMBIENT = {0.2, 0.8, 0.8};
  const S_Material COLOR_CYAN_DIFUSE = {0.2, 0.8, 0.8};
  const S_Material COLOR_CYAN_SPECULAR = {0.8, 0.8, 0.8};

  const S_Material COLOR_YELLOW_AMBIENT = {0.8, 0.8, 0.2};
  const S_Material COLOR_YELLOW_DIFUSE = {0.8, 0.8, 0.2};
  const S_Material COLOR_YELLOW_SPECULAR = {0.8, 0.8, 0.2};
  /* a vykreslime objekt ve stredu souradneho systemu */
  trScale(0.33, 0.33, 0.33);
  renderCurve(pRenderer, trajectory, COLOR_GREEN, 300);
  trScale(3, 3, 3);
  renderCurve(pRenderer, trajectory, COLOR_RED, 300);
  renderModel(pRenderer, pModel);

  renMatAmbient(pRenderer, &COLOR_CYAN_AMBIENT);
  renMatDiffuse(pRenderer, &COLOR_CYAN_DIFUSE);
  renMatSpecular(pRenderer, &COLOR_CYAN_SPECULAR);
  trTranslate(c1.x, c1.y, c1.z);
  trScale(2, 2, 2);
  renderModel(pRenderer, pModel);

  renMatAmbient(pRenderer, &COLOR_YELLOW_AMBIENT);
  renMatDiffuse(pRenderer, &COLOR_YELLOW_DIFUSE);
  renMatSpecular(pRenderer, &COLOR_YELLOW_SPECULAR);
  trScale(-0.33, 0.33, -0.33);
  trTranslate(c1.x, c1.y, c1.z);
  trScale(0.25, 0.25, 0.25);
  renderModel(pRenderer, pModel);
}

/*****************************************************************************
 *****************************************************************************/
