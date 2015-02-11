/*  RetroArch - A frontend for libretro.
 *  Copyright (C) 2011-2015 - Daniel De Matteis
 *  Copyright (C) 2014-2015 - Jean-André Santoni
 *
 *  RetroArch is free software: you can redistribute it and/or modify it under the terms
 *  of the GNU General Public License as published by the Free Software Found-
 *  ation, either version 3 of the License, or (at your option) any later version.
 *
 *  RetroArch is distributed in the hope that it will be useful, but WITHOUT ANY WARRANTY;
 *  without even the implied warranty of MERCHANTABILITY or FITNESS FOR A PARTICULAR
 *  PURPOSE.  See the GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License along with RetroArch.
 *  If not, see <http://www.gnu.org/licenses/>.
 */

#include "menu_texture.h"
#include <file/file_path.h>
#include "../general.h"
#include "../gfx/video_thread_wrapper.h"

#ifdef HAVE_OPENGL
#include "../gfx/gl_common.h"

static void menu_texture_png_load_gl(struct texture_image *ti,
      enum texture_filter_type filter_type,
      unsigned *id)
{
   /* Generate the OpenGL texture object */
   glGenTextures(1, id);
   glBindTexture(GL_TEXTURE_2D, (GLuint)*id);
   glTexImage2D(GL_TEXTURE_2D, 0, driver.gfx_use_rgba ?
         GL_RGBA : RARCH_GL_INTERNAL_FORMAT32,
         ti->width, ti->height, 0,
         driver.gfx_use_rgba ? GL_RGBA : RARCH_GL_TEXTURE_TYPE32,
         RARCH_GL_FORMAT32, ti->pixels);

   switch (filter_type)
   {
      case TEXTURE_FILTER_MIPMAP:
         glTexParameterf(GL_TEXTURE_2D,
               GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_NEAREST);
         glTexParameterf(GL_TEXTURE_2D,
               GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         glGenerateMipmap(GL_TEXTURE_2D);
         break;
      case TEXTURE_FILTER_DEFAULT:
      default:
         glTexParameterf(GL_TEXTURE_2D,
               GL_TEXTURE_MIN_FILTER, GL_LINEAR);
         glTexParameterf(GL_TEXTURE_2D,
               GL_TEXTURE_MAG_FILTER, GL_LINEAR);
         break;
   }
}
#endif

static unsigned menu_texture_png_load(const char *path,
      enum texture_backend_type type,
      enum texture_filter_type  filter_type)
{
   unsigned id = 0;
   struct texture_image ti = {0};
   if (! path_file_exists(path))
      return 0;

   texture_image_load(&ti, path);

   switch (type)
   {
      case TEXTURE_BACKEND_OPENGL:
#ifdef HAVE_OPENGL
         menu_texture_png_load_gl(&ti, filter_type, &id);
#endif
         break;
      case TEXTURE_BACKEND_DEFAULT:
      default:
         break;
   }

   free(ti.pixels);

   return id;
}

static int menu_texture_png_load_wrap(void *data)
{
   const char *filename = (const char*)data;
   if (!filename)
      return 0;
   return menu_texture_png_load(filename, TEXTURE_BACKEND_DEFAULT,
         TEXTURE_FILTER_DEFAULT);
}

static int menu_texture_png_load_wrap_gl_mipmap(void *data)
{
   const char *filename = (const char*)data;
   if (!filename)
      return 0;
   return menu_texture_png_load(filename, TEXTURE_BACKEND_OPENGL,
         TEXTURE_FILTER_MIPMAP);
}

static int menu_texture_png_load_wrap_gl(void *data)
{
   const char *filename = (const char*)data;
   if (!filename)
      return 0;
   return menu_texture_png_load(filename, TEXTURE_BACKEND_OPENGL,
         TEXTURE_FILTER_DEFAULT);
}

unsigned menu_texture_load(const char *path,
      enum texture_backend_type type,
      enum texture_filter_type  filter_type)
{
   if (g_settings.video.threaded
         && !g_extern.system.hw_render_callback.context_type)
   {
      thread_video_t *thr = (thread_video_t*)driver.video_data;

      if (!thr)
         return 0;

      switch (type)
      {
         case TEXTURE_BACKEND_OPENGL:
            if (filter_type == TEXTURE_FILTER_MIPMAP)
               thr->cmd_data.custom_command.method = menu_texture_png_load_wrap_gl_mipmap;
            else
               thr->cmd_data.custom_command.method = menu_texture_png_load_wrap_gl;
            break;
         case TEXTURE_BACKEND_DEFAULT:
         default:
            thr->cmd_data.custom_command.method = menu_texture_png_load_wrap;
            break;
      }

      thr->cmd_data.custom_command.data   = (void*)path;

      thr->send_cmd_func(thr, CMD_CUSTOM_COMMAND);
      thr->wait_reply_func(thr, CMD_CUSTOM_COMMAND);

      return thr->cmd_data.custom_command.return_value;
   }

   return menu_texture_png_load(path, type, filter_type);
}
