/* $Id: font_freetype.c,v 1.9 2002/10/13 13:26:16 micahjd Exp $
 *
 * font_freetype.c - Font engine that uses Freetype2 to render
 *                   spiffy antialiased Type1 and TrueType fonts
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>
#include <pgserver/font.h>
#include <pgserver/configfile.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <unistd.h>
#include <dirent.h>

#include <ft2build.h>
#include FT_FREETYPE_H
#include FT_CACHE_H

#define CFGSECTION "font-freetype"

#ifdef CONFIG_FREETYPE_GAMMA
#include <math.h>
u8 ft_light_gamma_table[256];
u8 ft_dark_gamma_table[256];
int ft_gamma_light_dark_threshold;     /* Premultiplied by 3 */
void ft_build_gamma_table(u8 *table, float gamma);
void ft_apply_gamma_table(u8 *table, FT_Bitmap *b);
u8 *ft_pick_gamma_table(hwrcolor c);
#endif

struct ft_face_id {
  const char *file_path;
  struct font_style fs;
  struct ft_face_id *next;
};

struct freetype_fontdesc {
  struct ft_face_id *face;
  int size;
  int flags;
};
#define DATA ((struct freetype_fontdesc *)self->data)

FT_Library         ft_lib;
struct ft_face_id* ft_facelist;
FT_Int32           ft_glyph_flags;
const char*        ft_default_name;
int                ft_default_size;
int                ft_minimum_size;
int                ft_dpi;
FTC_Manager        ft_cache;

/* Various bits turned on for matches in fontcmp.  The order
 * of these bits defines the priority of the various
 * attributes
 */
#define FCMP_TYPE     (1<<4)
#define FCMP_FIXEDVAR (1<<3)
#define FCMP_NAME     (1<<2)
#define FCMP_DEFAULT  (1<<1)
#define FCMP_STYLE    (1<<0)

void ft_face_scan(const char *directory);
void ft_face_load(const char *file);
int ft_fontcmp(const struct ft_face_id *f, const struct font_style *fs);
void ft_get_face_style(FT_Face f, struct font_style *fs);
void ft_get_descriptor_face(struct font_descriptor *self, FT_Face *aface);
static FT_Error ft_face_requester( FTC_FaceID   face_id,
				   FT_Library   library,
				   FT_Pointer   request_data,
				   FT_Face     *aface );


/************************************************* Initialization ***/

g_error freetype_engine_init(void) {
  g_error e;
  struct ft_face_id *f;
  
  if (FT_Init_FreeType(&ft_lib))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Use Freetype's cache system */
  if (FTC_Manager_New(ft_lib,get_param_int(CFGSECTION,"max_faces",0),
		      get_param_int(CFGSECTION,"max_faces",0),
		      get_param_int(CFGSECTION,"max_faces",0),
		      &ft_face_requester,NULL,&ft_cache))
    return mkerror(PG_ERRT_IO,119);   /* Error initializing font engine */

  /* Scan for available faces */
  ft_facelist = NULL;
  ft_face_scan(get_param_str(CFGSECTION,"path","/usr/share/fonts"));
  
  for (f=ft_facelist;f;f=f->next)
    printf("Indexed font file \"%s\" with family \"%s\", style %d\n",
	   f->file_path,f->fs.name,f->fs.style);

  if (!ft_facelist)
    return mkerror(PG_ERRT_IO,66);  /* Can't find fonts */

  ft_glyph_flags = 0;
  if (get_param_int(CFGSECTION,"no_hinting",0))
    ft_glyph_flags |= FT_LOAD_NO_HINTING;
  if (get_param_int(CFGSECTION,"no_bitmap",1))
    ft_glyph_flags |= FT_LOAD_NO_BITMAP;
  if (get_param_int(CFGSECTION,"force_autohint",1))
    ft_glyph_flags |= FT_LOAD_FORCE_AUTOHINT;
  if (get_param_int(CFGSECTION,"no_autohint",0))
    ft_glyph_flags |= FT_LOAD_NO_AUTOHINT;

  ft_default_size = get_param_int(CFGSECTION,"default_size",12);
  ft_default_name = get_param_str(CFGSECTION,"default_name","Helmet");
  ft_minimum_size = get_param_int(CFGSECTION,"minimum_size",5);
  ft_dpi = get_param_int(CFGSECTION,"dpi",72);

#ifdef CONFIG_FREETYPE_GAMMA
  ft_build_gamma_table(ft_light_gamma_table,
		       atof(get_param_str(CFGSECTION,"light_gamma","1.6")));
  ft_build_gamma_table(ft_dark_gamma_table,
		       atof(get_param_str(CFGSECTION,"dark_gamma","0.75")));
  ft_gamma_light_dark_threshold = 3*get_param_int(CFGSECTION,"gamma_light_dark_threshold",128);
#endif

  return success;
}

void freetype_engine_shutdown(void) {
  struct ft_face_id *f = ft_facelist;
  struct ft_face_id *condemn;

  while (f) {
    condemn = f;
    f = f->next;
    g_free(condemn->file_path);
    g_free(condemn->fs.name);
    g_free(condemn);
  }

  FTC_Manager_Done(ft_cache);
  FT_Done_FreeType(ft_lib);
}

static FT_Error ft_face_requester( FTC_FaceID   face_id,
				   FT_Library   library,
				   FT_Pointer   request_data,
				   FT_Face     *aface ) {
  struct ft_face_id *f = (struct ft_face_id *) face_id;
  
  return FT_New_Face( library, f->file_path, 0, aface );
}

/* Call ft_face_load for all fonts in a directory, recursively */
void ft_face_scan(const char *directory) {
  DIR *d = opendir(directory);
  struct dirent *dent;
  char buf[NAME_MAX];
  struct stat st;

  if (!d)
    return;

  while ((dent = readdir(d))) {
    /* Skip hidden files, "..", and "." */
    if (dent->d_name[0] == '.')
      continue;

    /* Find the path of this entry */
    buf[NAME_MAX-1] = 0;
    strncpy(buf,directory,NAME_MAX-2);
    strcat(buf,"/");
    strncat(buf,dent->d_name,NAME_MAX-1-strlen(buf));

    /* If it's a directory, recurse */
    if (stat(buf,&st) < 0)
      continue;
    if (S_ISDIR(st.st_mode)) {
      ft_face_scan(buf);
      continue;
    }

    /* Otherwise, try to load it */
    ft_face_load(buf);
  }

  closedir(d);
}

/* Store a ft_face_id for the specified font if it loads */
void ft_face_load(const char *file) {
  FT_Face face;
  struct ft_face_id *pfid;

  /* We must allocate the new node now (and free it if this fails)
   * since the face cache uses the _pointer_ as the cache key, not
   * the contents!
   */
  if (iserror(g_malloc((void**)&pfid, sizeof(struct ft_face_id))))
    return;

  pfid->file_path = file;
  if (FTC_Manager_Lookup_Face(ft_cache,(FTC_FaceID) pfid, &face)) {
    g_free(pfid);
    return;
  }

  /* Set up the face's style, making sure to dynamically allocate
   * the face name and path!
   */
  ft_get_face_style(face, &pfid->fs);
  if (iserror(g_malloc((void**)&pfid->file_path, strlen(file)+1)))
    return;
  if (iserror(g_malloc((void**)&pfid->fs.name, strlen(face->family_name)+1)))
    return;
  strcpy((char*)pfid->file_path,file);
  strcpy((char*)pfid->fs.name,face->family_name);

  pfid->next = ft_facelist;
  ft_facelist = pfid;
}


/************************************************* Rendering ***/

void freetype_draw_char(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
		   hwrcolor col, int ch, struct quad *clip, s16 lgop, s16 angle) {
  int i,j;
  FT_Bitmap b;
  FT_Face face;

  ft_get_descriptor_face(self,&face);
  FT_Load_Char(face,ch, FT_LOAD_RENDER | ft_glyph_flags);

  b = face->glyph->bitmap;
#ifdef CONFIG_FREETYPE_GAMMA
  ft_apply_gamma_table(ft_pick_gamma_table(col),&b);
#endif

  /* PicoGUI's character origin is at the top-left of the bounding box.
   * Add the ascent to reach the baseline, then subtract the bitmap origin
   * from that.
   */
  VID(alpha_charblit)(dest,b.buffer,
		      position->x + face->glyph->bitmap_left,
		      position->y + (face->size->metrics.ascender >> 6) -
		      face->glyph->bitmap_top,
		      b.width,b.rows,b.pitch,angle,col,clip,lgop);

  position->x += face->glyph->advance.x >> 6;
  position->y += face->glyph->advance.y >> 6;
}

/* Our own version of draw_string that handles subpixel pen movement 
 */
void freetype_draw_string(struct font_descriptor *self, hwrbitmap dest, struct pair *position,
			  hwrcolor col, const struct pgstring *str, struct quad *clip,
			  s16 lgop, s16 angle) {
  struct font_metrics m;
  struct pgstr_iterator p = PGSTR_I_NULL;
  int x,y,margin,lh,b,ch;
  FT_Face face;

  self->lib->getmetrics(self,&m);
  ft_get_descriptor_face(self,&face);

  x = position->x * 64;
  y = position->y * 64;
  margin = m.margin * 64;
  lh = face->size->metrics.height;

  switch (angle) {
    
  case 0:
    x += margin;
    y += margin;
    b = x;
    break;
    
  case 90:
    x += margin;
    y -= margin;
    b = y;
    break;
    
  case 180:
    x -= margin;
    y -= margin;
    b = x;
    break;

  case 270:
    x -= margin;
    y += margin;
    b = y;
    break;
    
  }
  
  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n')
      switch (angle) {
	
      case 0:
	y += lh;
	x = b;
	break;
	
      case 90:
	x += lh;
	y = b;
	break;
	
      case 180:
	y -= lh;
	x = b;
	break;
	
      case 270:
	x -= lh;
	y = b;
	break;
	
      }
    else if (ch!='\r') {
      position->x = x>>6;
      position->y = y>>6;
      self->lib->draw_char(self,dest,position,col,ch,clip,lgop,angle);

      /* Ignore the position modifications made by
       * this, do our own in subpixel units 
       */
      switch (angle) {
      case 0:
	x += face->glyph->advance.x;
	y += face->glyph->advance.y;
	break;
      case 90:
	x += face->glyph->advance.y;
	y -= face->glyph->advance.x;
	break;
      case 180:
	x -= face->glyph->advance.x;
	y -= face->glyph->advance.y;
	break;
      case 270:
	x -= face->glyph->advance.y;
	y += face->glyph->advance.x;
	break;
      }
    }
  }

  position->x = x >> 6;
  position->y = y >> 6;
}

#ifdef CONFIG_FREETYPE_GAMMA
void ft_build_gamma_table(u8 *table, float gamma) {
  int i;
  for (i=0;i<256;i++)
    table[i] = 255 * pow(i/255.0, 1/gamma);
}

void ft_apply_gamma_table(u8 *table, FT_Bitmap *b) {
  u8 *l,*p;
  int i,j;

  l = b->buffer;
  for (j=0;j<b->rows;j++,l+=b->pitch)
    for (i=0,p=l;i<b->width;i++,p++)
      *p = table[*p];
}

/* Use one table for light fonts, another for dark fonts */
u8 *ft_pick_gamma_table(hwrcolor c) {
  pgcolor pgc = vid->color_hwrtopg(c);
  if (getred(pgc)+getgreen(pgc)+getblue(pgc) >= ft_gamma_light_dark_threshold)
    return ft_light_gamma_table;
  return ft_dark_gamma_table;
}
#endif /* CONFIG_FREETYPE_GAMMA */


/************************************************* Font Descriptors ***/

g_error freetype_create(struct font_descriptor *self, const struct font_style *fs) {
  g_error e;
  int size;
  struct font_style defaultfs;
  int r, closeness = -1;
  struct ft_face_id *closest, *f;
  FT_Face face;

  e = g_malloc((void**)&self->data,sizeof(struct freetype_fontdesc));
  errorcheck;

  if (!fs) {
    fs = &defaultfs;
    defaultfs.name = ft_default_name;
    defaultfs.size = 0;
    defaultfs.style = PG_FSTYLE_DEFAULT;
    defaultfs.representation = 0;
  }

  /* Pick the closest font face */
  for (f=ft_facelist;f;f=f->next) {
    r = ft_fontcmp(f,fs);
    if (r > closeness) {
      closeness = r;
      closest = f;
    }
  }
  DATA->face = closest;

  DATA->size = fs->size ? fs->size : ft_default_size;
  if (DATA->size < ft_minimum_size)
    DATA->size = ft_minimum_size;
  DATA->flags = fs->style;

  return success;
}

void freetype_destroy(struct font_descriptor *self) {
  g_free(DATA);
}

/* Gauge the closeness between a face and a requested style
 */
int ft_fontcmp(const struct ft_face_id *f, const struct font_style *fs) {
  int result = 0;
  int szdif;
  struct font_style f_fs;

  if (fs->name && (!strcasecmp(f->fs.name,fs->name)))
    result |= FCMP_NAME;
  if ((f->fs.style&PG_FSTYLE_FIXED)==(fs->style&PG_FSTYLE_FIXED))
    result |= FCMP_FIXEDVAR;
  if ((f->fs.style&PG_FSTYLE_DEFAULT) && (fs->style&PG_FSTYLE_DEFAULT)) 
    result |= FCMP_DEFAULT;
  if ((f->fs.style&PG_FSTYLE_STYLE_MASK)==(fs->style&PG_FSTYLE_STYLE_MASK)) 
    result |= FCMP_STYLE;

  /*
  printf("Comparing potential %s:%d:%d to requested %s:%d:%d, closeness %d\n",
	 f->fs.name,f->fs.size,f->fs.style,fs->name,fs->size,fs->style,result);
  */

  return result;
}

void ft_get_descriptor_face(struct font_descriptor *self, FT_Face *aface) {
  static struct font_descriptor *last_fd = NULL;
  static FT_Face last_face = NULL;
  FTC_FontRec fr;

  /* Optimize for consecutive calls with the same face */
  if (self == last_fd) {
    *aface =  last_face;
    return;
  }

  fr.face_id = DATA->face;
  fr.pix_width = 0;
  fr.pix_height = DATA->size;
  FTC_Manager_Lookup_Size(ft_cache,&fr,aface,NULL);

  /* It should be safe to store the face here since this is the
   * only function that calls FTC_Manager_Lookup_Size
   */
  last_fd = self;
  last_face = *aface;
}

 
/************************************************* Metrics ***/

void freetype_measure_char(struct font_descriptor *self, struct pair *position,
		      int ch, s16 angle) {
  FT_Face face;
  ft_get_descriptor_face(self,&face);
  FT_Load_Char(face,ch, ft_glyph_flags);
  position->x += face->glyph->advance.x >> 6;
  position->y += face->glyph->advance.y >> 6;
}

void freetype_getmetrics(struct font_descriptor *self, struct font_metrics *m) {
  FT_Face face;
  ft_get_descriptor_face(self,&face);

  m->ascent = face->size->metrics.ascender >> 6;
  m->descent = (-face->size->metrics.descender) >> 6;
  m->lineheight = face->size->metrics.height >> 6;
  m->linegap = m->lineheight - m->ascent - m->descent;
  m->charcell.w = face->size->metrics.max_advance >> 6;
  m->charcell.h = m->ascent + m->descent;

  if (DATA->flags & PG_FSTYLE_FLUSH)
    m->margin = 0;
  else
    m->margin = m->descent;
}

/* Our own version of measure_string that handles subpixel pen movement 
 */
void freetype_measure_string(struct font_descriptor *self, const struct pgstring *str,
			     s16 angle, s16 *w, s16 *h) {
  struct font_metrics m;
  int max_x = 0, ch, x, y, original_x;
  struct pgstr_iterator p = PGSTR_I_NULL;
  FT_Face face;
  self->lib->getmetrics(self,&m);
  ft_get_descriptor_face(self,&face);

  original_x = x = m.margin << 7;
  y = x + face->size->metrics.height + face->size->metrics.descender;

  while ((ch = pgstring_decode(str,&p))) {
    if (ch=='\n') {
      y += face->size->metrics.height;
      if (x>max_x) max_x = x;
      x = original_x;
    }
    else if (ch!='\r') {
      FT_Load_Char(face,ch, ft_glyph_flags);
      x += face->glyph->advance.x;
      y += face->glyph->advance.y;
    }
  }
  if (x>max_x) max_x = x;

  if (angle==90 || angle==270) {
    *h = max_x >> 6;
    *w = y >> 6;
  }
  else {
    *w = max_x >> 6;
    *h = y >> 6;
  }
}


/************************************************* Enumeration ***/

void freetype_getstyle(int i, struct font_style *fs) {
  struct ft_face_id *f;

  /* Iterate to the face they asked about */
  for (f=ft_facelist;f && i>0;i--,f=f->next);
  if (!f) return;
  memcpy(fs,&f->fs,sizeof(*fs));
}

void ft_get_face_style(FT_Face f, struct font_style *fs) {
  memset(fs,0,sizeof(*fs));

  fs->name = f->family_name;

  if (f->style_flags & FT_STYLE_FLAG_ITALIC)    fs->style |= PG_FSTYLE_ITALIC;
  if (f->style_flags & FT_STYLE_FLAG_BOLD)      fs->style |= PG_FSTYLE_BOLD;
  if (f->face_flags & FT_FACE_FLAG_SCALABLE)    fs->representation |= PG_FR_SCALABLE;
  if (f->face_flags & FT_FACE_FLAG_FIXED_WIDTH) fs->style |= PG_FSTYLE_FIXED;

  /* Is there no better way to detect a condensed font? */
  if (f->style_name && strstr(f->style_name,"ondensed"))
    fs->style |= PG_FSTYLE_CONDENSED;
}


/************************************************* Registration ***/

g_error freetype_regfunc(struct fontlib *f) {
  f->engine_init = &freetype_engine_init;
  f->engine_shutdown = &freetype_engine_shutdown;
  f->draw_char = &freetype_draw_char;
  f->measure_char = &freetype_measure_char;
  f->create = &freetype_create;
  f->destroy = &freetype_destroy;
  f->getstyle = &freetype_getstyle;
  f->getmetrics = &freetype_getmetrics;
  f->measure_string = &freetype_measure_string;
  f->draw_string = &freetype_draw_string;
  return success;
}

/* The End */

