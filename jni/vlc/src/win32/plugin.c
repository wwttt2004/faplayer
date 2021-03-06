/*****************************************************************************
 * plugin.c : Low-level dynamic library handling
 *****************************************************************************
 * Copyright (C) 2001-2011 the VideoLAN team
 *
 * Authors: Sam Hocevar <sam@zoy.org>
 *          Ethan C. Baldridge <BaldridgeE@cadmus.com>
 *          Hans-Peter Jansen <hpj@urpla.net>
 *          Gildas Bazin <gbazin@videolan.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston MA 02110-1301, USA.
 *****************************************************************************/

#ifdef HAVE_CONFIG_H
# include "config.h"
#endif

#include <vlc_common.h>
#include <vlc_charset.h>
#include "modules/modules.h"
#include <windows.h>
#include <wchar.h>

static char *GetWindowsError( void )
{
    wchar_t wmsg[256];
    int i = 0, i_error = GetLastError();

    FormatMessageW( FORMAT_MESSAGE_FROM_SYSTEM | FORMAT_MESSAGE_IGNORE_INSERTS,
                    NULL, i_error, MAKELANGID (LANG_NEUTRAL, SUBLANG_DEFAULT),
                    wmsg, 256, NULL );

    /* Go to the end of the string */
    while( !wmemchr( L"\r\n\0", wmsg[i], 3 ) )
        i++;

    snwprintf( wmsg + i, 256 - i, L" (error %i)", i_error );
    return FromWide( wmsg );
}

int module_Load( vlc_object_t *p_this, const char *psz_file,
                 module_handle_t *p_handle, bool lazy )
{
    wchar_t *wfile = ToWide (psz_file);
    if (wfile == NULL)
        return -1;

    module_handle_t handle;
#ifndef UNDER_CE
    /* FIXME: this is not thread-safe -- Courmisch */
    UINT mode = SetErrorMode (SEM_FAILCRITICALERRORS);
    SetErrorMode (mode|SEM_FAILCRITICALERRORS);
#endif

    handle = LoadLibraryW (wfile);

#ifndef UNDER_CE
    SetErrorMode (mode);
#endif
    free (wfile);

    if( handle == NULL )
    {
        char *psz_err = GetWindowsError();
        msg_Warn( p_this, "cannot load module `%s' (%s)", psz_file, psz_err );
        free( psz_err );
        return -1;
    }

    *p_handle = handle;
    (void) lazy;
    return 0;
}

void module_Unload( module_handle_t handle )
{
    FreeLibrary( handle );
}

void *module_Lookup( module_handle_t handle, const char *psz_function )
{
#ifdef UNDER_CE
    wchar_t wide[strlen( psz_function ) + 1];
    size_t i = 0;
    do
        wide[i] = psz_function[i]; /* UTF-16 <- ASCII */
    while( psz_function[i++] );

    return (void *)GetProcAddress( handle, wide );

#else
    return (void *)GetProcAddress( handle, (char *)psz_function );

#endif
}
