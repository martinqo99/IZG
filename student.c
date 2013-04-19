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

/* pocet kroku pri vykreslovani krivky */
const int           CURVE_QUALITY   = 100;

/* dalsi globalni promenne a konstanty */
/* ??? */


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
    /* ??? */
    
    return -1;
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
    /* ??? */
}


/******************************************************************************
 * Vykresli caru zadanou barvou
 * Pred vykreslenim aplikuje na koncove body cary aktualne nastavene
 * transformacni matice!
 * p1, p2 - koncove body
 * color - kreslici barva */

void studrenProjectLine(S_Renderer *pRenderer, S_Coords *p1, S_Coords *p2, S_RGBA color)
{
    /* ??? */
}

/******************************************************************************
 * Funkce pro vykresleni krivky jako "lomene cary"
 * color - kreslici barva
 * n - pocet bodu na krivce, kolik se ma pouzit pro aproximaci */

void renderCurve(S_Renderer *pRenderer, S_Curve *pCurve, S_RGBA color, int n)
{
    /* ??? */
}

/******************************************************************************
 * Callback funkce volana pri startu aplikace */

void onInit()
{
    /* vytvoreni a inicializace krivky */
    /* ??? */
}

/******************************************************************************
 * Callback funkce volana pri tiknuti casovace */

void onTimer(int ticks)
{
    /* uprava pozice planetky */
    /* ??? */
}

/******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte pro kresleni dynamicke sceny, ve ktere se "planetky"
 * pohybuji po krivce, tj. mirne elipticke draze. */

void renderScene(S_Renderer *pRenderer, S_Model *pModel)
{
    /* ??? */

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

    /* a vykreslime objekt ve stredu souradneho systemu */
    renderModel(pRenderer, pModel);

    /* ??? */
}


/*****************************************************************************
 *****************************************************************************/
