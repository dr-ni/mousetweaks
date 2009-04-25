/*
 * Copyright © 2007-2009 Gerd Kohlberger <lowfi@chello.at>
 *
 * This file is part of Mousetweaks.
 *
 * Mousetweaks is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 3 of the License, or
 * (at your option) any later version.
 *
 * Mousetweaks is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program. If not, see <http://www.gnu.org/licenses/>.
 */

#include "mt-accessible.h"

#define MAX_SEARCHES 200

gboolean
mt_accessible_is_visible (Accessible *accessible)
{
    AccessibleStateSet *states;
    gboolean visible = FALSE;

    if (accessible) {
	states = Accessible_getStateSet (accessible);
	visible = AccessibleStateSet_contains (states, SPI_STATE_VISIBLE) &&
		  AccessibleStateSet_contains (states, SPI_STATE_SHOWING);
	AccessibleStateSet_unref (states);
    }
    return visible;
}

gboolean
mt_accessible_is_active (Accessible *accessible)
{
    AccessibleStateSet *states;
    gboolean active = FALSE;

    if (accessible) {
	states = Accessible_getStateSet (accessible);
	active = AccessibleStateSet_contains (states, SPI_STATE_ACTIVE);
	AccessibleStateSet_unref (states);
    }
    return active;
}

gboolean
mt_accessible_supports_action (Accessible  *accessible,
			       const gchar *action_name)
{
    AccessibleAction *action;
    gboolean support = FALSE;
    glong i, n;
    gchar *name;

    g_return_val_if_fail (action_name != NULL, FALSE);

    if (accessible && Accessible_isAction (accessible)) {
	action = Accessible_getAction (accessible);
	n = AccessibleAction_getNActions (action);

	for (i = 0; i < n; ++i) {
	    name = AccessibleAction_getName (action, i);
	    support = g_str_equal (name, action_name);
	    SPI_freeString (name);

	    if (support)
		break;
	}
	AccessibleAction_unref (action);
    }
    return support;
}

gboolean
mt_accessible_get_extents (Accessible *accessible,
			   SPIRect    *extents)
{
    AccessibleComponent *component;

    g_return_val_if_fail (extents != NULL, FALSE);

    if (accessible && Accessible_isComponent (accessible)) {
	component = Accessible_getComponent (accessible);
	AccessibleComponent_getExtents (component,
					&extents->x,
					&extents->y,
					&extents->width,
					&extents->height,
					SPI_COORD_TYPE_SCREEN);
	AccessibleComponent_unref (component);
	return TRUE;
    }
    return FALSE;
}

gboolean
mt_accessible_in_extents (Accessible *accessible, gint x, gint y)
{
    AccessibleComponent *component;
    gboolean in = FALSE;

    if (accessible && Accessible_isComponent (accessible)) {
	component = Accessible_getComponent (accessible);
	in = AccessibleComponent_contains (component, x, y,
					   SPI_COORD_TYPE_SCREEN);
	AccessibleComponent_unref (component);
    }
    return in;
}

gboolean
mt_accessible_point_in_rect (SPIRect rectangle, glong x, glong y)
{
    return x >= rectangle.x &&
	   y >= rectangle.y &&
	   x <= (rectangle.x + rectangle.width) &&
	   y <= (rectangle.y + rectangle.height);
}

Accessible *
mt_accessible_search (Accessible  *accessible,
		      MtSearchType type,
		      MtSearchFunc eval,
		      MtSearchFunc push,
		      gpointer     data)
{
    GQueue *queue;
    Accessible *a;
    gboolean found;
    gint n_searches;
    glong i;

    g_return_val_if_fail (accessible != NULL, NULL);

    queue = g_queue_new ();
    g_queue_push_head (queue, accessible);
    Accessible_ref (accessible);

    a = NULL;
    n_searches = 0;
    found = FALSE;

    if (type == MT_SEARCH_TYPE_BREADTH) {
	/* (reverse) breadth first search - queue FIFO */
	while (!g_queue_is_empty (queue)) {
	    a = g_queue_pop_tail (queue);

	    if (!a)
		continue;

	    if ((found = (eval) (a, data)))
		break;
	    else if (++n_searches >= MAX_SEARCHES) {
		Accessible_unref (a);
		break;
	    }

	    if ((push) (a, data))
		for (i = 0; i < Accessible_getChildCount (a); ++i)
		    g_queue_push_head (queue, Accessible_getChildAtIndex (a, i));

	    Accessible_unref (a);
	}
    }
    else if (type == MT_SEARCH_TYPE_DEPTH) {
	/* depth first search - queue FILO */
	while (!g_queue_is_empty (queue)) {
	    a = g_queue_pop_head (queue);

	    if (!a)
		continue;

	    if ((found = (eval) (a, data)))
		break;
	    else if (++n_searches >= MAX_SEARCHES) {
		Accessible_unref (a);
		break;
	    }

	    if ((push) (a, data))
		for (i = 0; i < Accessible_getChildCount (a); ++i)
		    g_queue_push_head (queue, Accessible_getChildAtIndex (a, i));

	    Accessible_unref (a);
	}
    }
    else
	g_warning ("Unknown search type.");

    g_queue_foreach (queue, (GFunc) Accessible_unref, NULL);
    g_queue_free (queue);

    return found ? a : NULL;
}

Accessible *
mt_accessible_at_point (gint x, gint y)
{
    Accessible *desk, *app, *frame, *a;
    AccessibleComponent *component;
    glong n_app, n_child;
    gint i, j;

    a = NULL;
    desk = SPI_getDesktop (0);
    n_app = Accessible_getChildCount (desk);

    for (i = 0; i < n_app; ++i) {
	app = Accessible_getChildAtIndex (desk, i);
	if (!app)
	    continue;

	n_child = Accessible_getChildCount (app);
	for (j = 0; j < n_child; ++j) {
	    frame = Accessible_getChildAtIndex (app, j);
	    if (!frame)
		continue;

	    if (!Accessible_getRole (frame) == SPI_ROLE_FRAME ||
		!mt_accessible_is_visible (frame) ||
		!Accessible_isComponent (frame)) {
		Accessible_unref (frame);
		continue;
	    }

	    component = Accessible_getComponent (frame);
	    a = AccessibleComponent_getAccessibleAtPoint (component, x, y,
							  SPI_COORD_TYPE_SCREEN);
	    AccessibleComponent_unref (component);
	    Accessible_unref (frame);

	    if (a)
		break;
	}
	Accessible_unref (app);

	if (a)
	    break;
    }
    Accessible_unref (desk);

    return a;
}
