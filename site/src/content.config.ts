import { defineCollection } from 'astro:content';
import { z } from 'astro/zod';
import { docsLoader } from '@astrojs/starlight/loaders';
import { docsSchema } from '@astrojs/starlight/schema';
import { genreIds } from './genres';

export const collections = {
	docs: defineCollection({
		loader: docsLoader(),
		// At most two, so the icons fit next to the name in the sidebar.
		schema: docsSchema({ extend: z.object({ genres: z.array(z.enum(genreIds)).max(2).optional() }) }),
	}),
};
