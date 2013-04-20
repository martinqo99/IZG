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

S_Coords 			CURVE;
double 				TRAJECTORY = 0.0f;


// Barvicky
S_RGBA 				COLOR_GREY = {127, 127, 127, 255};
S_RGBA 				COLOR_GREY2 = {200, 200, 200, 255};

// Materialy
S_Material 			MAT_GOLD_AMBIENT = {0.24725, 0.1995, 0.0745};
S_Material 			MAT_GOLD_DIFFUSE = {0.75164, 0.60648, 0.22648};
S_Material 			MAT_GOLD_SPECULAR = {0.774597, 0.774597, 0.774597};

S_Material 			MAT_EMERALD_AMBIENT = {0.0215, 0.1745, 0.0215};
S_Material 			MAT_EMERALD_DIFFUSE = {0.07568, 0.61424, 0.07568};
S_Material 			MAT_EMERALD_SPECULAR = {0.628281, 0.555802, 0.366065};


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

int studrenCullLine(S_Renderer *pRenderer, int *x1, int *y1, int *x2, int *y2, double *u1, double *u2){
	IZG_ASSERT(pRenderer && x1 && y1 && x2 && y2 && u1 && u2);
	
	// Parametricke vyjadreni primky
	// x = x1 + u * dx
	// y = y1 + u * dy
	
	int Xmin = 1;
	int Ymin = 1;
	int Xmax = pRenderer->frame_w - 2;
	int Ymax = pRenderer->frame_h - 2;
	
	// Ziskavani delta
	int dx = *x2 - *x1;
	int dy = *y2 - *y1;
	
	// Nastaveni defaultnich hodnot u1 - min, u2 - max (interval 0 - 1)
	*u1 = 0.0f;
	*u2 = 1.0f;
	
	int pi[4];
	int qi[4];
	
	double tmp;
	
	// Nastavovani pi
	pi[0] = -dx;
	pi[1] = dx;
	pi[2] = -dy;
	pi[3] = dy;
	
	// Nastavovani qi
	qi[0] = *x1 - Xmin;
	qi[1] = Xmax - *x1;
	qi[2] = *y1 - Ymin;
	qi[3] = Ymax - *y1;
	
	int i;
	for(i = 0; i < 4; i++){
		tmp = 0.0f;
		
		// Pokud neni primka vodorovna, nastavime parametr (nedelime 0)
		if(pi[i] != 0.0f)
			tmp = (double) qi[i] / (double) pi[i];		
		
		// Smeruje dovnitr oblasti
		if(pi[i] < 0){
			if(tmp > *u2)
				return -1;
			else if(tmp > *u1)
				*u1 = tmp;			
		}
		// Smeruje ven z oblasti
		else if(pi[i] > 0){
			if(tmp < *u1)
				return -1;
			else if(tmp < *u2)
				*u2 = tmp;			
		}
		// Lezi pred hranici, je mozne ji vynechat
		else if(pi[i] == 0 && qi[i] < 0)
			return -1;		
	}
	
	// Vypocitani novych primek (jiz v oblasti)
	if(*u2 < 1){
		*x2 = *x1 + ROUND(*u2 * dx);
		*y2 = *y1 + ROUND(*u2 * dy);		
	}
	if(*u1 > 0){
		*x1 = *x1 + ROUND(*u1 * dx);
		*y1 = *y1 + ROUND(*u1 * dy);
	}	
    
    return 0;
}

/****************************************************************************
 * Rasterizace usecky do frame bufferu s interpolaci z-souradnice
 * a zapisem do z-bufferu
 * v1, v2 - ukazatele na kocove body usecky ve 3D prostoru pred projekci
 * x1, y1, x2, y2 - souradnice koncovych bodu ve 2D po projekci
 * color - kreslici barva */

void studrenDrawLine(S_Renderer *pRenderer, S_Coords *v1, S_Coords *v2, int x1, int y1, int x2, int y2, S_RGBA color){

	double u1,u2;

	if(studrenCullLine(pRenderer, &x1, &y1, &x2, &y2,&u1,&u2) == -1)
		return;

	// Inspirovano cvicenim c. 2
	int dx = ABS(x2 - x1);
	int dy = ABS(y2 - y1);
	int dz = ABS(v2->z - v1->z) / ((dx == 0)? 1 : dx);
	
	int transponed = 0;
	
	// Zamena os
	if(dy > dx){
		transponed = 1;
		SWAP(x1, y1);
		SWAP(x2, y2);
		SWAP(dx, dy);		
	}
	
	// Prohozeni os
	if(x1 > x2){
		SWAP(x1, x2);
		SWAP(y1, y2);		
	}
	
	int stepY = 1;
	
	if(y1 > y2)
		stepY = -1;
	
	// BH konstanty
	int k1 = 2 * dy;
	int k2 = 2 * (dy - dx);
	int p  = 2 * dy - dx;
	
	int x, y, z;
	for(x = x1, y = y1; x <= x2; x++){
		z = (x - x1) * dz + v1->z;
		
		if(transponed == 1 && z < DEPTH(pRenderer, y, x)){
			DEPTH(pRenderer, y, x) = z;
			PIXEL(pRenderer, y, x) = color;			
		}
		else if(transponed == 0 && z < DEPTH(pRenderer, x, y)){
			DEPTH(pRenderer, x, y) = z;
			PIXEL(pRenderer, x, y) = color;			
		}

		if(p < 0)
			p += k1;
		else{
			p += k2;
			y += stepY;			
		}		
	}
}

/******************************************************************************
 * Vykresli caru zadanou barvou
 * Pred vykreslenim aplikuje na koncove body cary aktualne nastavene
 * transformacni matice!
 * p1, p2 - koncove body
 * color - kreslici barva */

void studrenProjectLine(S_Renderer *pRenderer, S_Coords *p1, S_Coords *p2, S_RGBA color){
	
	IZG_ASSERT(pRenderer && p1 && p2);
	
	// Souradnice ve 3D
	S_Coords x, y;
	
	trTransformVertex(&x, p1);
	trTransformVertex(&y, p2);
	
	// Body na obrazovce
	int x1, y1, x2, y2;
	
	trProjectVertex(&x1, &y1, &x);
	trProjectVertex(&x2, &y2, &y);
	
	studrenDrawLine(pRenderer, &x, &y, x1, y1, x2, y2, color);
}

/******************************************************************************
 * Funkce pro vykresleni krivky jako "lomene cary"
 * color - kreslici barva
 * n - pocet bodu na krivce, kolik se ma pouzit pro aproximaci */

void renderCurve(S_Renderer *pRenderer, S_Curve *pCurve, S_RGBA color, int n){
	S_Coords a, b;	
	double step;
	double t;
	
	step = t = 1.0/n;
	
	for(a = curvePoint(pCurve, 0.0); t < 1; t += step){
		
		b = curvePoint(pCurve, t);
		
		studrenProjectLine(pRenderer, &a, &b, color);
		
		a = b;
	}
	
	a = curvePoint(pCurve, 0.0);
	studrenProjectLine(pRenderer, &a, &b, color);
}

/******************************************************************************
 * Callback funkce volana pri startu aplikace */

void onInit(){
  /* vytvoreni a inicializace krivky */
  trajectory = curveCreate();
  curveInit(trajectory, 7, 2);

  cvecGet(trajectory->points, 0) = makeCoords(5, 0, 0);
  dvecGet(trajectory->knots, 0) = 0.0;
  dvecGet(trajectory->weights, 0) = 1.0;
  
  dvecGet(trajectory->knots, 1) = 0.0;
  cvecGet(trajectory->points, 1) = makeCoords(5, 0, -5);
  dvecGet(trajectory->weights, 1) = 0.5;
  
  dvecGet(trajectory->knots, 2) = 0.0;
  cvecGet(trajectory->points, 2) = makeCoords(-5, 0, -5);
  dvecGet(trajectory->weights, 2) = 0.5;
  
  dvecGet(trajectory->knots, 3) = 0.25;
  cvecGet(trajectory->points, 3) = makeCoords(-5, 0, 0);
  dvecGet(trajectory->weights, 3) = 1.0;
  
  dvecGet(trajectory->knots, 4) = 0.5;
  cvecGet(trajectory->points, 4) = makeCoords(-5, 0, 5);
  dvecGet(trajectory->weights, 4) = 0.5;
  
  dvecGet(trajectory->knots, 5) = 0.5;
  cvecGet(trajectory->points, 5) = makeCoords(5, 0, 5);
  dvecGet(trajectory->weights, 5) = 0.5;
  
  dvecGet(trajectory->knots, 6) = 0.75;
  cvecGet(trajectory->points, 6) = makeCoords(5, 0, 0);
  dvecGet(trajectory->weights, 6) = 1.0;
}

/******************************************************************************
 * Callback funkce volana pri tiknuti casovace */

void onTimer(int ticks){
	CURVE = curvePoint(trajectory, TRAJECTORY);
	
	TRAJECTORY += 0.01;
	
	if(TRAJECTORY > 1.00)
		TRAJECTORY = 0.00;	
}

/******************************************************************************
 * Funkce pro vyrenderovani sceny, tj. vykresleni modelu
 * Upravte pro kresleni dynamicke sceny, ve ktere se "planetky"
 * pohybuji po krivce, tj. mirne elipticke draze. */

void renderScene(S_Renderer *pRenderer, S_Model *pModel){

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
	
	//
	// Taaak a ted vykresluju ja!!!
	//
	
	// Prvni trajektorie
	trScale(0.25, 0.25, 0.25);
	renderCurve(pRenderer, trajectory, COLOR_GREY, CURVE_QUALITY);
	
	// Druha trajektorie
	trScale(2, 2, 2);
	renderCurve(pRenderer, trajectory, COLOR_GREY, CURVE_QUALITY);
	
	// Toz vykreslime je ne?
	renderModel(pRenderer, pModel);
	
	// Prvni obihajici objekt
	renMatAmbient(pRenderer, &MAT_GOLD_AMBIENT);
    renMatDiffuse(pRenderer, &MAT_GOLD_DIFFUSE);
    renMatSpecular(pRenderer, &MAT_GOLD_SPECULAR);
	
	trTranslate(CURVE.x, CURVE.y, CURVE.z);
	trScale(0.75, 0.75, 0.75);
	renderModel(pRenderer, pModel);
	
	// Posunuti
	trScale(-0.675, 0.675, -0.675);
	
	// Druhy obihajici objekt
	renMatAmbient(pRenderer, &MAT_EMERALD_AMBIENT);
    renMatDiffuse(pRenderer, &MAT_EMERALD_DIFFUSE);
    renMatSpecular(pRenderer, &MAT_EMERALD_SPECULAR);		
	
	trTranslate(CURVE.x, CURVE.y, CURVE.z);
	trScale(0.5, 0.5, 0.5);
	renderModel(pRenderer, pModel);
}


/*****************************************************************************
 *****************************************************************************/
