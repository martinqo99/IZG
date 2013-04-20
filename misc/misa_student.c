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
S_Curve             * trajectory    = NULL;
S_Curve             * trajectory2   = NULL;

/* pocet kroku pri vykreslovani krivky */
const int           CURVE_QUALITY   = 100;

/* dalsi globalni promenne a konstanty */
S_Coords coords1;
S_Coords coords2;
float t1 = 0;
float t2 = 0.825;

const S_Material    COLOR_BLUEE_AMBIENT  = { 0.2, 0.2, 0.8 };
const S_Material    COLOR_BLUEE_DIFFUSE  = { 0.2, 0.2, 0.8 };
const S_Material    COLOR_BLUEE_SPECULAR = { 0.8, 0.8, 0.8 };

const S_Material    COLOR_GREEEN_AMBIENT  = { 0.2, 0.8, 0.2 };
const S_Material    COLOR_GREEEN_DIFFUSE  = { 0.2, 0.8, 0.2 };
const S_Material    COLOR_GREEEN_SPECULAR = { 0.8, 0.8, 0.8 };

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
    *u1 = 0.0;
    *u2 = 1.0;

    /* postupne proverime pi a qi */
    for( i = 0; i < 4; ++i )
    {
        /* vypocet parametru ri */
        ri = 0.0f;
        if( p[i] != 0.0f )
        {
            ri = (float)q[i] / (float)p[i];
        }

        /* primka smeruje do okna */
        if( p[i] < 0 )
        {
            if( ri > *u2 ) return -1;
            else if( ri > *u1 ) *u1 = ri;
        }
        else if( p[i] > 0 )     /* primka smeruje z okna ven */
        {
            if( ri < *u1 ) return -1;
            else if( ri < *u2 ) *u2 = ri;
            
        }
        else if( q[i] < 0 )     /* primka je mimo okno */
        {
            return -1;
        }
    }

    /* spocteme vysledne souradnice x1,y1 a x2,y2 primky */
    if( *u2 < 1 )
    {
        t1 = *u2 * dx;
        t2 = *u2 * dy;
        *x2 = *x1 + ROUND(t1);
        *y2 = *y1 + ROUND(t2);
    }
    if( *u1 > 0 )
    {
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

  int dz,dx, dy, xy_swap, step_y, k1, k2, p, x, y,z, tmp;
  double u1,u2;

  /* orezani do okna */
  if( studrenCullLine(pRenderer, &x1, &y1, &x2, &y2,&u1,&u2) == -1 ){
    return;
  }

  /* delta v ose x a y */
  dx = ABS(x2 - x1);
  dy = ABS(y2 - y1);
  if (x2-x1 == 0 )
    tmp = 1;
  else
    tmp = x2-x1;
  dz = ABS(v1->z - v2->z) / (tmp);
  //printf("%d",dz);
  
  /* zamena osy x a y v algoritmu vykreslovani */
  xy_swap = 0;
  if( dy > dx ){
    xy_swap = 1;
    SWAP(x1, y1);
    SWAP(x2, y2);
    SWAP(dx, dy);
  }

  /* pripadne prohozeni bodu v ose x */
  if( x1 > x2 ){
    SWAP(x1, x2);
    SWAP(y1, y2);
  }

/* smer postupu v ose y */
  if( y1 > y2 ) step_y = -1;
  else step_y = 1;
  
  /* konstanty pro Bresenhamuv algoritmus */
  k1 = 2 * dy;
  k2 = 2 * (dy - dx);
  p = 2 * dy - dx;
  /* pocatecni prirazeni hodnot */
  x = x1;
  y = y1;
  /* cyklus vykreslovani usecky */
  for( ; x <= x2; ++x ){
    z = v1->z + (x - x1) * dz;
    if( xy_swap ) {
      if(z<DEPTH(pRenderer, y, x)){
        DEPTH(pRenderer, y, x) = z;
        PIXEL(pRenderer, y, x) = color;
      }
    }
    else {
      if(z<DEPTH(pRenderer, x, y)){
        DEPTH(pRenderer, x, y) = z;
        PIXEL(pRenderer, x, y) = color;
      }
    }
  /* uprava prediktoru */
  if( p < 0 ){
    p += k1;
  }
  else{
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
    S_Coords co1, co2;
    float step = 1.0/n;
    float t = 0.0;
    co1=curvePoint(pCurve, t);
    while(t<=1){
      co2=curvePoint(pCurve, t);
      studrenProjectLine(pRenderer, &co1, &co2, color);
      t+=step;
      co1=co2;
    }
}

/******************************************************************************
 * Callback funkce volana pri startu aplikace */

void onInit()
{
    trajectory = curveCreate();
    curveInit(trajectory,7,2);
    
  cvecGet(trajectory->points, 0) = makeCoords(-5,0,0);
  cvecGet(trajectory->points, 1) = makeCoords(-5,0,5);
  cvecGet(trajectory->points, 2) = makeCoords(5,0,5);
  cvecGet(trajectory->points, 3) = makeCoords(5,0,0);
  cvecGet(trajectory->points, 4) = makeCoords(5,0,-5);
  cvecGet(trajectory->points, 5) = makeCoords(-5,0,-5);
  cvecGet(trajectory->points, 6) = makeCoords(-5,0,0);
  
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
  
  trajectory2 = curveCreate();
  curveInit(trajectory2,7,2);
    
  cvecGet(trajectory2->points, 0) = makeCoords(-7.5,0,0);
  cvecGet(trajectory2->points, 1) = makeCoords(-7.5,0,7.5);
  cvecGet(trajectory2->points, 2) = makeCoords(7.5,0,7.5);
  cvecGet(trajectory2->points, 3) = makeCoords(7.5,0,0);
  cvecGet(trajectory2->points, 4) = makeCoords(7.5,0,-7.5);
  cvecGet(trajectory2->points, 5) = makeCoords(-7.5,0,-7.5);
  cvecGet(trajectory2->points, 6) = makeCoords(-7.5,0,0);
  
  dvecGet(trajectory2->knots, 0) = 0.0;
  dvecGet(trajectory2->knots, 1) = 0.0;
  dvecGet(trajectory2->knots, 2) = 0.0;
  dvecGet(trajectory2->knots, 3) = 0.25;
  dvecGet(trajectory2->knots, 4) = 0.5;
  dvecGet(trajectory2->knots, 5) = 0.5;
  dvecGet(trajectory2->knots, 6) = 0.75;
  dvecGet(trajectory2->knots, 7) = 1.0;
  dvecGet(trajectory2->knots, 8) = 1.0;
  dvecGet(trajectory2->knots, 9) = 1.0;
  dvecGet(trajectory2->weights, 0) = 1.0;
  dvecGet(trajectory2->weights, 1) = 0.5;
  dvecGet(trajectory2->weights, 2) = 0.5;
  dvecGet(trajectory2->weights, 3) = 1.0;
  dvecGet(trajectory2->weights, 4) = 0.5;
  dvecGet(trajectory2->weights, 5) = 0.5;
  dvecGet(trajectory2->weights, 6) = 1.0;
}
  
/******************************************************************************
 * Callback funkce volana pri tiknuti casovace */

void onTimer(int ticks)
{
    
    coords1 = curvePoint(trajectory, t1);
    t1 += 0.01; //bylo 0.05
    if (t1 > 0.998){
      t1 = 0;
    }
    coords2 = curvePoint(trajectory2, t2);
    t2 -= 0.01; //0.005
    if (t2 < 0.01)
      t2 = 0.999;
}


/******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte pro kresleni dynamicke sceny, ve ktere se "planetky"
 * pohybuji po krivce, tj. mirne elipticke draze. */

void renderScene(S_Renderer *pRenderer, S_Model *pModel)
{
    S_RGBA color;

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
    renMatSpecular(pRenderer, &MAT_RED_SPECULAR);
    renMatDiffuse(pRenderer, &MAT_RED_DIFFUSE);
    /* a vykreslime objekt ve stredu souradneho systemu */
    color = COLOR_BLUE;
    S_Matrix mymatrix;
    renderCurve(pRenderer, trajectory, color, 100);
    renderCurve(pRenderer, trajectory2, COLOR_RED, 100);
    renderModel(pRenderer, pModel);    
    
    trGetMatrix(&mymatrix);
    renMatAmbient(pRenderer, &COLOR_GREEEN_AMBIENT);
    renMatSpecular(pRenderer, &COLOR_GREEEN_SPECULAR);
    renMatDiffuse(pRenderer, &COLOR_GREEEN_DIFFUSE);
    
    trTranslate(coords1.x,coords1.y,coords1.z);
    trScale(0.5,0.5,0.5);
    renderModel(pRenderer, pModel);
    trSetMatrix(&mymatrix);
    
    trTranslate(coords2.x,coords2.y,coords2.z);
    trScale(1.5,1.5,1.5);
    renMatAmbient(pRenderer, &COLOR_BLUEE_AMBIENT);
    renMatSpecular(pRenderer, &COLOR_BLUEE_SPECULAR);
    renMatDiffuse(pRenderer, &COLOR_BLUEE_DIFFUSE);
    renderModel(pRenderer, pModel);
    trSetMatrix(&mymatrix);
    /* ??? */
}


/*****************************************************************************
 *****************************************************************************/
