/* 
A* -------------------------------------------------------------------
B* This file contains source code for the PyMOL computer program
C* copyright 1998-2000 by Warren Lyford Delano of DeLano Scientific. 
D* -------------------------------------------------------------------
E* It is unlawful to modify or remove this copyright notice.
F* -------------------------------------------------------------------
G* Please see the accompanying LICENSE file for further information. 
H* -------------------------------------------------------------------
I* Additional authors of this source file include:
-* 
-* 
-*
Z* -------------------------------------------------------------------
*/

#include"os_predef.h"
#include"os_std.h"
#include"os_gl.h"

#include"OOMac.h"
#include"ObjectMesh.h"
#include"Base.h"
#include"MemoryDebug.h"
#include"Map.h"
#include"Debug.h"
#include"Parse.h"
#include"Isosurf.h"
#include"Vector.h"
#include"Color.h"
#include"main.h"
#include"Scene.h"
#include"Setting.h"
#include"Executive.h"
#include"PConv.h"
#include"P.h"

ObjectMesh *ObjectMeshNew(void);

static void ObjectMeshFree(ObjectMesh *I);
void ObjectMeshStateInit(ObjectMeshState *ms);
void ObjectMeshRecomputeExtent(ObjectMesh *I);


static PyObject *ObjectMeshStateAsPyList(ObjectMeshState *I)
{
  PyObject *result = NULL;

  result = PyList_New(14);
  
  PyList_SetItem(result,0,PyInt_FromLong(I->Active));
  PyList_SetItem(result,1,PyString_FromString(I->MapName));
  PyList_SetItem(result,2,PyInt_FromLong(I->MapState));
  PyList_SetItem(result,3,CrystalAsPyList(&I->Crystal));
  PyList_SetItem(result,4,PyInt_FromLong(I->ExtentFlag));
  PyList_SetItem(result,5,PConvFloatArrayToPyList(I->ExtentMin,3));
  PyList_SetItem(result,6,PConvFloatArrayToPyList(I->ExtentMax,3));
  PyList_SetItem(result,7,PConvIntArrayToPyList(I->Range,6));
  PyList_SetItem(result,8,PyFloat_FromDouble(I->Level));
  PyList_SetItem(result,9,PyFloat_FromDouble(I->Radius));
  PyList_SetItem(result,10,PyInt_FromLong(I->CarveFlag));
  PyList_SetItem(result,11,PyFloat_FromDouble(I->CarveBuffer));
  if(I->CarveFlag&&I->AtomVertex) {
    PyList_SetItem(result,12,PConvFloatVLAToPyList(I->AtomVertex));
  } else {
    PyList_SetItem(result,12,PConvAutoNone(NULL));
  }
  PyList_SetItem(result,13,PyInt_FromLong(I->DotFlag));

#if 0
  char MapName[ObjNameMax];
  int MapState;
  CCrystal Crystal;
  int Active;
  int *N;
  float *V;
  int Range[6];
  float ExtentMin[3],ExtentMax[3];
  int ExtentFlag;
  float Level,Radius;
  int RefreshFlag;
  int ResurfaceFlag;
  float *AtomVertex;
  int CarveFlag;
  float CarveBuffer;
  int DotFlag;
  CGO *UnitCellCGO;
#endif

  return(PConvAutoNone(result));  
}


static PyObject *ObjectMeshAllStatesAsPyList(ObjectMesh *I)
{
  
  PyObject *result=NULL;
  int a;
  result = PyList_New(I->NState);
  for(a=0;a<I->NState;a++) {
    if(I->State[a].Active) {
      PyList_SetItem(result,a,ObjectMeshStateAsPyList(I->State+a));
    } else {
      PyList_SetItem(result,a,PConvAutoNone(NULL));
    }
  }
  return(PConvAutoNone(result));  

}

static int ObjectMeshStateFromPyList(ObjectMeshState *I,PyObject *list)
{
  int ok=true;
  int ll;
  PyObject *tmp;
  if(ok) ok=(list!=NULL);
  if(ok) {
    if(!PyList_Check(list))
      I->Active=false;
    else {
      ObjectMeshStateInit(I);
      if(ok) ok=(list!=NULL);
      if(ok) ok=PyList_Check(list);
      if(ok) ll = PyList_Size(list);
      /* TO SUPPORT BACKWARDS COMPATIBILITY...
         Always check ll when adding new PyList_GetItem's */
      
      if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,0),&I->Active);
      if(ok) ok = PConvPyStrToStr(PyList_GetItem(list,1),I->MapName,ObjNameMax);
      if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,2),&I->MapState);
      if(ok) ok = CrystalFromPyList(&I->Crystal,PyList_GetItem(list,3));
      if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,4),&I->ExtentFlag);
      if(ok) ok = PConvPyListToFloatArrayInPlace(PyList_GetItem(list,5),I->ExtentMin,3);
      if(ok) ok = PConvPyListToFloatArrayInPlace(PyList_GetItem(list,6),I->ExtentMax,3);
      if(ok) ok = PConvPyListToIntArrayInPlace(PyList_GetItem(list,7),I->Range,6);
      if(ok) ok = PConvPyFloatToFloat(PyList_GetItem(list,8),&I->Level);
      if(ok) ok = PConvPyFloatToFloat(PyList_GetItem(list,9),&I->Radius);
      if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,10),&I->CarveFlag);
      if(ok) ok = PConvPyFloatToFloat(PyList_GetItem(list,11),&I->CarveBuffer);
      if(ok) {
        tmp = PyList_GetItem(list,12);
        if(tmp == Py_None)
          I->AtomVertex = NULL;
        else 
          ok = PConvPyListToFloatVLA(tmp,&I->AtomVertex);
      }
      if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,13),&I->DotFlag);
      if(ok) {
        I->RefreshFlag=true;
        I->ResurfaceFlag=true;
      }
    }
  }
  return(ok);
}

static int ObjectMeshAllStatesFromPyList(ObjectMesh *I,PyObject *list)
{
  int ok=true;
  int a;
  VLACheck(I->State,ObjectMeshState,I->NState);
  if(ok) ok=PyList_Check(list);
  if(ok) {
    for(a=0;a<I->NState;a++) {
      ok = ObjectMeshStateFromPyList(I->State+a,PyList_GetItem(list,a));
      if(!ok) break;
    }
  }
  return(ok);
}

int ObjectMeshNewFromPyList(PyObject *list,ObjectMesh **result)
{
  int ok = true;
  int ll;
  ObjectMesh *I=NULL;
  (*result) = NULL;
  
  if(ok) ok=(list!=NULL);
  if(ok) ok=PyList_Check(list);
  if(ok) ll = PyList_Size(list);
  /* TO SUPPORT BACKWARDS COMPATIBILITY...
     Always check ll when adding new PyList_GetItem's */

  I=ObjectMeshNew();
  if(ok) ok = (I!=NULL);
  
  if(ok) ok = ObjectFromPyList(PyList_GetItem(list,0),&I->Obj);
  if(ok) ok = PConvPyIntToInt(PyList_GetItem(list,1),&I->NState);
  if(ok) ok = ObjectMeshAllStatesFromPyList(I,PyList_GetItem(list,2));
  if(ok) {
    (*result) = I;
    ObjectMeshRecomputeExtent(I);
  } else {
    /* cleanup? */
  }
  return(ok);
}



PyObject *ObjectMeshAsPyList(ObjectMesh *I)
{
  
  PyObject *result=NULL;

  result = PyList_New(3);
  PyList_SetItem(result,0,ObjectAsPyList(&I->Obj));
  PyList_SetItem(result,1,PyInt_FromLong(I->NState));
  PyList_SetItem(result,2,ObjectMeshAllStatesAsPyList(I));

  return(PConvAutoNone(result));  
}

static void ObjectMeshStateFree(ObjectMeshState *ms)
{
  if(PMGUI) {
    if(ms->displayList) {
      if(PIsGlutThread()) {
        glDeleteLists(ms->displayList,1);
        ms->displayList = 0;
      } else {
        char buffer[255]; /* pass this off to the main thread */
        sprintf(buffer,"_cmd.gl_delete_lists(%d,%d)\n",ms->displayList,1);
        PParse(buffer);
      }
    }
  }
  VLAFreeP(ms->N);
  VLAFreeP(ms->V);
  VLAFreeP(ms->AtomVertex);
  if(ms->UnitCellCGO)
    CGOFree(ms->UnitCellCGO);
}

static void ObjectMeshFree(ObjectMesh *I) {
  int a;
  for(a=0;a<I->NState;a++) {
    if(I->State[a].Active)
      ObjectMeshStateFree(I->State+a);
  }
  VLAFreeP(I->State);
  ObjectPurge(&I->Obj);
  
  OOFreeP(I);
}

void ObjectMeshDump(ObjectMesh *I,char *fname,int state)
{
  float *v;
  int *n;
  int c;
  FILE *f;
  f=fopen(fname,"wb");
  if(!f) 
    ErrMessage("ObjectMeshDump","can't open file for writing");
  else {
    if(state<I->NState) {
      n=I->State[state].N;
      v=I->State[state].V;
      if(n&&v)
        while(*n)
          {
            c=*(n++);
            if(!I->State[state].DotFlag) {
              fprintf(f,"\n");
            }
            while(c--) {
              fprintf(f,"%10.4f%10.4f%10.4f\n",v[0],v[1],v[2]);
              v+=3;
            }
          }
    }
    fclose(f);
    PRINTFB(FB_ObjectMesh,FB_Actions)
      " ObjectMeshDump: %s written to %s\n",I->Obj.Name,fname
      ENDFB;
  }
}

static void ObjectMeshInvalidate(ObjectMesh *I,int rep,int level,int state)
{
  int a;
  int once_flag=true;
  for(a=0;a<I->NState;a++) {
    if(state<0) once_flag=false;
    if(!once_flag) state=a;
    I->State[state].RefreshFlag=true;
    if(level>=cRepInvAll) {
      I->State[state].ResurfaceFlag=true;      
      SceneChanged();
    } else {
      SceneDirty();
    }

    if(once_flag) break;
  }
}

int ObjectMeshSetLevel(ObjectMesh *I,float level,int state)
{
  int a;
  int ok=true;
  int once_flag=true;
  ObjectMeshState *ms;
  if(state>=I->NState) {
    ok=false;
  } else {
    for(a=0;a<I->NState;a++) {
      if(state<0) {
        once_flag=false;
      }
      if(!once_flag) {
        state = a;
      }
      ms = I->State + state;
      if(ms->Active) {
        ms->ResurfaceFlag=true;
        ms->RefreshFlag=true;
        ms->Level = level;
      }
      if(once_flag) {
        break;
      }
    }
  }
  return(ok);
}

static void ObjectMeshUpdate(ObjectMesh *I) 
{
  int a;
  int c;
  ObjectMeshState *ms;
  ObjectMapState *oms = NULL;
  ObjectMap *map = NULL;

  int *n;
  float *v;
  int *old_n;
  float *old_v;
  int n_cur;
  int n_seg;
  int n_line;
  int flag;
  int last_flag=0;
  int h,k,l;
  int i,j;
  int ok=true;
  MapType *voxelmap; /* this has nothing to do with isosurfaces... */
  
  for(a=0;a<I->NState;a++) {
    ms = I->State+a;
    if(ms->Active) {
      
      map = ExecutiveFindObjectMapByName(ms->MapName);
      if(!map) {
        ok=false;
        PRINTFB(FB_ObjectMesh,FB_Errors)
          "ObjectMeshUpdate-Error: map '%s' has been deleted.\n",ms->MapName
          ENDFB;
        ms->ResurfaceFlag=false;
      }
      if(map) {
        oms = ObjectMapGetState(map,ms->MapState);
        if(!oms) ok=false;
      }
      if(oms) {
        if(ms->RefreshFlag||ms->ResurfaceFlag) {
          ms->Crystal = *(oms->Crystal);
          if(I->Obj.RepVis[cRepCell]) {
            if(ms->UnitCellCGO)
              CGOFree(ms->UnitCellCGO);
            ms->UnitCellCGO = CrystalGetUnitCellCGO(&ms->Crystal);
          } 
          ms->RefreshFlag=false;
        }
      }
      
      if(map&&oms&&ms->N&&ms->V&&I->Obj.RepVis[cRepMesh]) {
        if(ms->ResurfaceFlag) {
          ms->ResurfaceFlag=false;
          PRINTF " ObjectMesh: updating \"%s\".\n" , I->Obj.Name ENDF;
          if(oms->Field) {
            IsosurfGetRange(oms->Field,oms->Crystal,
                            ms->ExtentMin,ms->ExtentMax,ms->Range);
            /*          printf("%d %d %d %d %d %d\n",
                   ms->Range[0],
                   ms->Range[1],
                   ms->Range[2],
                   ms->Range[3],
                   ms->Range[4],
                   ms->Range[5]);*/
                
            IsosurfVolume(oms->Field,
                          ms->Level,
                          &ms->N,&ms->V,
                          ms->Range,
                          ms->DotFlag); 
          }
          if(ms->CarveFlag&&ms->AtomVertex&&
             VLAGetSize(ms->N)&&VLAGetSize(ms->V)) {
            /* cull my friend, cull */
            voxelmap=MapNew(-ms->CarveBuffer,ms->AtomVertex,VLAGetSize(ms->AtomVertex)/3,NULL);
            if(voxelmap) {
              
              MapSetupExpress(voxelmap);  
              
              old_n = ms->N;
              old_v = ms->V;
              ms->N=VLAlloc(int,VLAGetSize(old_n));
              ms->V=VLAlloc(float,VLAGetSize(old_v));
              
              n = old_n;
              v = old_v;
              n_cur = 0;
              n_seg = 0;
              n_line = 0;
              while(*n)
                {
                  last_flag=false;
                  c=*(n++);
                  while(c--) {
                    flag=false;
                    MapLocus(voxelmap,v,&h,&k,&l);
                    i=*(MapEStart(voxelmap,h,k,l));
                    if(i) {
                      j=voxelmap->EList[i++];
                      while(j>=0) {
                        if(within3f(ms->AtomVertex+3*j,v,ms->CarveBuffer)) {
                          flag=true;
                          break;
                        }
                        j=voxelmap->EList[i++];
                      }
                    }
                    if(flag&&(!last_flag)) {
                      VLACheck(ms->V,float,3*(n_line+1));
                      copy3f(v,ms->V+n_line*3);
                      n_cur++;
                      n_line++;
                    }
                    if(flag&&last_flag) { /* continue segment */
                      VLACheck(ms->V,float,3*(n_line+1));
                      copy3f(v,ms->V+n_line*3);
                      n_cur++;
                      n_line++;
                    } if((!flag)&&last_flag) { /* terminate segment */
                      VLACheck(ms->N,int,n_seg);
                      ms->N[n_seg]=n_cur;
                      n_seg++;
                      n_cur=0;
                    }
                    last_flag=flag;
                    v+=3;
                  }
                  if(last_flag) { /* terminate segment */
                    VLACheck(ms->N,int,n_seg);
                    ms->N[n_seg]=n_cur;
                    n_seg++;
                    n_cur=0;
                  }
                }
              VLACheck(ms->N,int,n_seg);
              ms->N[n_seg]=0;
              VLAFreeP(old_n);
              VLAFreeP(old_v);
              MapFree(voxelmap);
            }
          }
        }
      }
    }
    SceneDirty();
  }
}

static void ObjectMeshRender(ObjectMesh *I,int state,CRay *ray,Pickable **pick,int pass)
{
  float *v = NULL;
  float *vc;
  int *n = NULL;
  int c;
  int a=0;
  ObjectMeshState *ms = NULL;

  ObjectPrepareContext(&I->Obj,ray);
  
  if(state>=0) 
    if(state<I->NState) 
      if(I->State[state].Active)
        if(I->State[state].V&&I->State[state].N)
          ms=I->State+state;

  while(1) {
    if(state<0) { /* all_states */
      ms = I->State + a;
    } else {
      if(!ms) {
        if(I->NState&&
           ((SettingGet(cSetting_static_singletons)&&(I->NState==1))))
          ms=I->State;
      }
    }
    if(ms) {
      if(ms->Active&&ms->V&&ms->N) {
        v=ms->V;
        n=ms->N;
        if(ray) {
          
          if(ms->UnitCellCGO&&(I->Obj.RepVis[cRepCell]))
            CGORenderRay(ms->UnitCellCGO,ray,ColorGet(I->Obj.Color),
                         I->Obj.Setting,NULL);
          ms->Radius=SettingGet_f(I->Obj.Setting,NULL,cSetting_mesh_radius);
          if(n&&v&&I->Obj.RepVis[cRepMesh]) {
            vc = ColorGet(I->Obj.Color);
            if(ms->DotFlag) {
              ray->fColor3fv(ray,vc);
              while(*n)
                {
                  c=*(n++);
                  if(c--)
                    {
                      v+=3;
                      while(c--)
                        {
                          ray->fSphere3fv(ray,v,ms->Radius);
                          v+=3;
                        }
                    }
                }
            } else {
              while(*n)
                {
                  c=*(n++);
                  if(c--)
                    {
                      v+=3;
                      while(c--)
                        {
                          ray->fSausage3fv(ray,v-3,v,ms->Radius,vc,vc);
                          v+=3;
                        }
                    }
                }
            }
          }
        } else if(pick&&PMGUI) {
        } else if(PMGUI) {
          if(!pass) {

            int use_dlst;
            if(ms->UnitCellCGO&&(I->Obj.RepVis[cRepCell]))
              CGORenderGL(ms->UnitCellCGO,ColorGet(I->Obj.Color),
                          I->Obj.Setting,NULL);

            SceneResetNormal(false);
            ObjectUseColor(&I->Obj);
            use_dlst = (int)SettingGet(cSetting_use_display_lists);
            if(use_dlst&&ms->displayList) {
              glCallList(ms->displayList);
            } else { 
              
              if(use_dlst) {
                if(!ms->displayList) {
                  ms->displayList = glGenLists(1);
                  if(ms->displayList) {
                    glNewList(ms->displayList,GL_COMPILE_AND_EXECUTE);
                  }
                }
              }

            if(n&&v&&I->Obj.RepVis[cRepMesh]) {
              glLineWidth(SettingGet_f(I->Obj.Setting,NULL,cSetting_mesh_width));
              while(*n)
                {
                  c=*(n++);
                  if(ms->DotFlag) 
                    glBegin(GL_POINTS);
                  else 
                    glBegin(GL_LINE_STRIP);
                  while(c--) {
                    glVertex3fv(v);
                    v+=3;
                  }
                  glEnd();
                }
            }
            if(use_dlst&&ms->displayList) {
              glEndList();
            }
                  
            }
          }
        }
      }
    }
    if(state>=0) break;
    a = a + 1;
    if(a>=I->NState) break;
  }
}

/*========================================================================*/

static int ObjectMeshGetNStates(ObjectMesh *I) 
{
  return(I->NState);
}

/*========================================================================*/
ObjectMesh *ObjectMeshNew(void)
{
  OOAlloc(ObjectMesh);
  
  ObjectInit((CObject*)I);
  
  I->NState = 0;
  I->State=VLAMalloc(10,sizeof(ObjectMeshState),5,true); /* autozero important */

  I->Obj.type = cObjectMesh;
  
  I->Obj.fFree = (void (*)(struct CObject *))ObjectMeshFree;
  I->Obj.fUpdate =  (void (*)(struct CObject *)) ObjectMeshUpdate;
  I->Obj.fRender =(void (*)(struct CObject *, int, CRay *, Pickable **,int ))ObjectMeshRender;
  I->Obj.fInvalidate =(void (*)(struct CObject *,int,int,int))ObjectMeshInvalidate;
  I->Obj.fGetNFrame = (int (*)(struct CObject *)) ObjectMeshGetNStates;
  return(I);
}

/*========================================================================*/
void ObjectMeshStateInit(ObjectMeshState *ms)
{
  if(!ms->V) {
    ms->V = VLAlloc(float,10000);
  }
  if(!ms->N) {
    ms->N = VLAlloc(int,10000);
  }
  if(ms->AtomVertex) {
    VLAFreeP(ms->AtomVertex);
  }
  ms->N[0]=0;
  ms->Active=true;
  ms->ResurfaceFlag=true;
  ms->ExtentFlag=false;
  ms->CarveFlag=false;
  ms->CarveBuffer=0.0;
  ms->AtomVertex=NULL;
  ms->UnitCellCGO=NULL;
  ms->displayList=0;
}

/*========================================================================*/
ObjectMesh *ObjectMeshFromBox(ObjectMesh *obj,ObjectMap *map,
                              int map_state,
                              int state,float *mn,float *mx,
                              float level,int dotFlag,
                              float carve,float *vert_vla)
{
  ObjectMesh *I;
  ObjectMeshState *ms;
  ObjectMapState *oms;

  if(!obj) {
    I=ObjectMeshNew();
  } else {
    I=obj;
  }

  if(state<0) state=I->NState;
  if(I->NState<=state) {
    VLACheck(I->State,ObjectMeshState,state);
    I->NState=state+1;
  }

  ms=I->State+state;
  ObjectMeshStateInit(ms);

  strcpy(ms->MapName,map->Obj.Name);
  ms->MapState = map_state;
  oms = ObjectMapGetState(map,map_state);

  ms->Level = level;
  ms->DotFlag = dotFlag;
  if(oms) {
    copy3f(mn,ms->ExtentMin); /* this is not exactly correct...should actually take vertex points from range */
    copy3f(mx,ms->ExtentMax);
    IsosurfGetRange(oms->Field,oms->Crystal,mn,mx,ms->Range);
    ms->ExtentFlag = true;
  }
  if(carve>=0.0) {
    ms->CarveFlag=true;
    ms->CarveBuffer = carve;
    ms->AtomVertex = vert_vla;
  }
  if(I) {
    ObjectMeshRecomputeExtent(I);
  }
  I->Obj.ExtentFlag=true;
  /*  printf("Brick %d %d %d %d %d %d\n",I->Range[0],I->Range[1],I->Range[2],I->Range[3],I->Range[4],I->Range[5]);*/
  SceneChanged();
  SceneCountFrames();
  return(I);
}

/*========================================================================*/

void ObjectMeshRecomputeExtent(ObjectMesh *I)
{
  int extent_flag = false;
  int a;
  ObjectMeshState *ms;

  for(a=0;a<I->NState;a++) {
    ms=I->State+a;
    if(ms->Active) {
      if(ms->ExtentFlag) {
        if(!extent_flag) {
          extent_flag=true;
          copy3f(ms->ExtentMax,I->Obj.ExtentMax);
          copy3f(ms->ExtentMin,I->Obj.ExtentMin);
        } else {
          max3f(ms->ExtentMax,I->Obj.ExtentMax,I->Obj.ExtentMax);
          min3f(ms->ExtentMin,I->Obj.ExtentMin,I->Obj.ExtentMin);
        }
      }
    }
  }
  I->Obj.ExtentFlag=extent_flag;
}

