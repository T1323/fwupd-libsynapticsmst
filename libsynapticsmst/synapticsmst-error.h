/* -*- Mode: C; tab-width: 8; indent-tabs-mode: t; c-basic-offset: 8 -*-
 *
 * Copyright (C) 2015 Richard Hughes <richard@hughsie.com>
 *
 * Licensed under the GNU Lesser General Public License Version 2.1
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301 USA
 */

#ifndef __SYNAPTICSMST_ERROR_H
#define __SYNAPTICSMST_ERROR_H

#include <glib.h>

#define SYNAPTICSMST_ERROR			synapticsmst_error_quark()

/**
 * SynapticsMSTError:
 * @SYNAPTICSMST_ERROR_INTERNAL:				Internal error
 *
 * The error code.
 **/
typedef enum {
	SYNAPTICSMST_ERROR_INTERNAL,			/* Since: 0.7.5 */
	/*< private >*/
	SYNAPTICSMST_ERROR_LAST
} SynapticsMSTError;

GQuark		 synapticsmst_error_quark			(void);

#endif /* __SYNAPTICSMST_ERROR_H */
