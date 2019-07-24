#define USAGEFMT "\n\
Usage:\n\
\n\
  %s [[UTC1[ UTC2[ ...]]] meta_kernel1.tm[ MK2.tm[ ...]]] [-h]\n\
\n\
  N.B. Meta-Kernel (MK)filenames ***MUST*** end in '.tm'\n\
\n\
  N.B. Arguments are processed in reverse order.\n\
\n\
  Example (using PDS NH SPICE archive; see Kernel Management below): \n\
\n\
    %s 2015-07-14T11:58:{2..0}{9..0} 2015-07-14T11:55:{5..0}{9..0} nh_pcnh_002_pck.tm nhsp_1000/extras/mk/nh_v04.tm\n\
\n\
\n\
Kernel Management (setup for the example above):\n\
\n\
  wget https://naif.jpl.nasa.gov/pub/naif/pds/data/nh-j_p_ss-spice-6-v1.0/nhsp_1000/ \\\n\
  -X /pub/naif/pds/data/nh-j_p_ss-spice-6-v1.0/nhsp_1000/catalog \\\n\
  -X /pub/naif/pds/data/nh-j_p_ss-spice-6-v1.0/nhsp_1000/document \\\n\
  -X /pub/naif/pds/data/nh-j_p_ss-spice-6-v1.0/nhsp_1000/index \\\n\
  -X /pub/naif/pds/data/nh-j_p_ss-spice-6-v1.0/nhsp_1000/software \\\n\
  -R '*.lbl,*=*' \\\n\
  --mirror -np -nH --cut-dirs=5 -N\n\
\n\
  ln -s nhsp_1000/data   ### Make SPICE archive data/ sub-directory apper \"local\"\n\
\n\
  *** N.B. You may need to run that WGET command twice to get everything\n\
\n\
"

/* Caveats
     You may want to look at sincpt_c, which may reduce much of this to a few lines, plus declarations e.g.
     Call furnsh_c to load kernels
     Call utc2et_c to get ET
     surfpt_c("ELLIPSOID","PLUTO",et,"IAU_PLUTO","LT","NH",v_bore_rex,"NH_REX",v_intercept_point,&et_minus_lt,&found);
 */

#include <stdio.h>
#include <string.h>
#include "SpiceUsr.h"

void utc_to_intercept(SpiceChar* utc) {
SpiceInt rex_id;
SpiceBoolean found;
SpiceDouble et;
SpiceDouble et_minus_lt;
SpiceChar shape[99];
SpiceChar frame[99];
SpiceDouble v_bore_rex[3];           /* REX boresight, REX instrument frame */
SpiceInt n;
SpiceDouble bounds[1][3];
SpiceDouble pluto_state[6];
SpiceDouble v_nh_plutobff[3];           /* NH position, Pluto body-fixed frame (BFF) */
SpiceDouble lighttime;
SpiceDouble v_bore_pluto[3];         /* REX boresight, Pluto BFF */
SpiceDouble mtx_rex_to_pluto[3][3];  /* Rotate REX to Pluto */
SpiceDouble abc[3];                  /* Pluto radii */
SpiceDouble v_intercept_point[3];

  /* Convert NH_REX frame name to ID; get REX boresight from Instrument Kernel (IK) */
  bods2c_c("NH_REX",&rex_id,&found);
  if (!found) {
    printf("NH_REX frame ID not found\n");
    return;
  }
  getfov_c(rex_id,1,99,99,shape,frame,v_bore_rex,&n,bounds);

  /* Get Pluto RADII */
  gdpool_c("BODY999_RADII",0,3,&n,abc,&found);
  if (!found) {
    printf("Body 999 not found in PCA\n");
    return;
  }

  /* Convert UTC to ET */
  utc2et_c(utc,&et);

  /* Get vector from NH to Pluto, corrected for light travel time from Pluto to NH */
  spkezr_c("PLUTO",et,"IAU_PLUTO","LT","NH",pluto_state,&lighttime);

  et_minus_lt = et - lighttime;

  /* Get rotation matrix from REX at ET to Pluto BFF ET-LIGHTTIME */
  pxfrm2_c("NH_REX","IAU_PLUTO",et,et_minus_lt,mtx_rex_to_pluto);

  /* Convert REX boresight vector to Pluto BFF */
  mxv_c(mtx_rex_to_pluto,v_bore_rex,v_bore_pluto);

  /* Invert NH-relative Pluto position, from first three SpiceDoubles of 6-element pluto_state to get Pluto-relative NH position */
  vminus_c(pluto_state,v_nh_plutobff);

  /* Call SURFPT to get intersection, if any */
  surfpt_c(v_nh_plutobff,v_bore_pluto,abc[0],abc[1],abc[2],v_intercept_point,&found);

  /* Output one CSV line with results */
  {
  SpiceDouble rrd[3];
  SpiceDouble v_intercept_to_nh[3];
  SpiceDouble v_intercept_normal[3];
  SpiceDouble emission;
  static int first = 1;
    if (found) {
      /* Intercept found; calculate related values */
      /* - R, East Longitude, and Latitude of intercept */
      recrad_c(v_intercept_point,rrd+0,rrd+1,rrd+2);
      rrd[1] *= dpr_c();
      rrd[2] *= dpr_c();

      /* - Vector from intercept to NH */
      vsub_c(v_intercept_point,v_nh_plutobff,v_intercept_to_nh);

      /* - Surface normal at intercept */
      vsub_c(v_nh_plutobff,v_intercept_point,v_intercept_to_nh);
      surfnm_c(abc[0],abc[1],abc[2],v_intercept_point,v_intercept_normal);

      /* - Emission angle to NH */
      emission = dpr_c() * vsep_c(v_intercept_normal,v_intercept_to_nh);

    } else {
      /* Use unrealistic values if intercept was not found */
      v_intercept_point[0] = v_intercept_point[1] = v_intercept_point[2] = 0.0;
      v_intercept_to_nh[0] = v_intercept_to_nh[1] = v_intercept_to_nh[2] = 0.0;
      rrd[0] = rrd[1] = rrd[2] = -999.0;
      emission = -999.0;
    }

    if (first) {
      /* Header line for CSV output */
      fprintf(stdout,"UTC,intercept found,V_intercept_X,_Y,_Z,Intercept_R,_ELon,_Lat,emission,V_intercept_to_nh,_Y,_Z,V_nh_X,_Y,_Z\n");
      first = 0;
    }

    /* CSV output */
    fprintf(stdout,"\"%s\",\"%s\",%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf,%.3lf\r\n"
                  ,utc
                  ,found ? "True" : "False"
                  ,v_intercept_point[0],v_intercept_point[1],v_intercept_point[2]
                  ,rrd[0],rrd[1],rrd[2]
                  ,emission
                  ,v_intercept_to_nh[0],v_intercept_to_nh[1],v_intercept_to_nh[2]
                  ,v_nh_plutobff[0],v_nh_plutobff[1],v_nh_plutobff[2]
                  );
  }

  return;
}

int main(int argc, char** argv) {


  /* Loop over command-line arguments, right to left */
  while (--argc) {

#   define ARG argv[argc]

    if (!strcmp(ARG,"-h")) {
      /* -h => Usage */
      argc = 0;
      fprintf(stdout,USAGEFMT,ARG,ARG);
      return 0;
    }

    if (strstr(ARG,".tm") == (ARG + strlen(ARG) - 3)) {
      /* File ending in ".tm" must be a meta-kernel; FURNSH it */
      furnsh_c(ARG);
      continue;
    }

    /* Any other argument must be a UTC; process it */
    utc_to_intercept(ARG);

  }
  return 0;
}
