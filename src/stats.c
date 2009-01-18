/* the Music Player Daemon (MPD)
 * Copyright (C) 2003-2007 by Warren Dukes (warren.dukes@gmail.com)
 * Copyright (C) 2008 Max Kellermann <max@duempel.org>
 * This project's homepage is: http://www.musicpd.org
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
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA
 */

#include "stats.h"
#include "database.h"
#include "tag.h"
#include "song.h"
#include "client.h"
#include "player_control.h"
#include "strset.h"

struct stats stats;

void stats_global_init(void)
{
	stats.timer = g_timer_new();
}

void stats_global_finish(void)
{
	g_timer_destroy(stats.timer);
}

struct visit_data {
	struct strset *artists;
	struct strset *albums;
};

static void
visit_tag(struct visit_data *data, const struct tag *tag)
{
	if (tag->time > 0)
		stats.song_duration += tag->time;

	for (unsigned i = 0; i < tag->numOfItems; ++i) {
		const struct tag_item *item = tag->items[i];

		switch (item->type) {
		case TAG_ITEM_ARTIST:
			strset_add(data->artists, item->value);
			break;

		case TAG_ITEM_ALBUM:
			strset_add(data->albums, item->value);
			break;

		default:
			break;
		}
	}
}

static int
stats_collect_song(struct song *song, void *_data)
{
	struct visit_data *data = _data;

	++stats.song_count;

	if (song->tag != NULL)
		visit_tag(data, song->tag);

	return 0;
}

void stats_update(void)
{
	struct visit_data data;

	stats.song_count = 0;
	stats.song_duration = 0;
	stats.artist_count = 0;

	data.artists = strset_new();
	data.albums = strset_new();

	db_walk(NULL, stats_collect_song, NULL, &data);

	stats.artist_count = strset_size(data.artists);
	stats.album_count = strset_size(data.albums);

	strset_free(data.artists);
	strset_free(data.albums);
}

int stats_print(struct client *client)
{
	client_printf(client,
		      "artists: %u\n"
		      "albums: %u\n"
		      "songs: %i\n"
		      "uptime: %li\n"
		      "playtime: %li\n"
		      "db_playtime: %li\n"
		      "db_update: %li\n",
		      stats.artist_count,
		      stats.album_count,
		      stats.song_count,
		      (long)g_timer_elapsed(stats.timer, NULL),
		      (long)(getPlayerTotalPlayTime() + 0.5),
		      stats.song_duration,
		      db_get_mtime());
	return 0;
}
