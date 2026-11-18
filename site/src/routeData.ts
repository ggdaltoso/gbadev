import { getCollection } from 'astro:content';
import { defineRouteMiddleware, type StarlightRouteData } from '@astrojs/starlight/route-data';
import { genre, genreUrl, type GenreId } from './genres';

const byHref = new Map<string, GenreId[]>();
for (const page of await getCollection('docs')) {
	if (page.data.genres) byHref.set(`/${page.id}/`, page.data.genres);
}

// Puts each page's genre icons on its sidebar link, as background images drawn by custom.css.
function tag(entries: StarlightRouteData['sidebar']) {
	for (const entry of entries) {
		if (entry.type === 'group') {
			tag(entry.entries);
			continue;
		}
		const ids = byHref.get(entry.href);
		if (!ids) continue;
		const names = ids.map((id) => genre(id).name).join(', ');
		entry.attrs = {
			...entry.attrs,
			'data-genres': '',
			title: names,
			'aria-label': `${entry.label} (${names})`,
			style: `--genre-icons: ${ids.map(genreUrl).join(', ')}; --genre-count: ${ids.length}; --genre-positions: ${ids.map((_, i) => `${i * 1.25}em 0`).join(', ')}`,
		};
	}
}

export const onRequest = defineRouteMiddleware((context) => {
	tag(context.locals.starlightRoute.sidebar);
});
