// @ts-check
import { defineConfig } from 'astro/config';
import starlight from '@astrojs/starlight';

// https://astro.build/config
export default defineConfig({
	site: 'https://gbadev.ggdaltoso.dev',
	integrations: [
		starlight({
			title: 'gbadev',
			social: [{ icon: 'github', label: 'GitHub', href: 'https://github.com/ggdaltoso/gbadev' }],
			sidebar: [
				{
					label: 'The basics',
					items: [{ autogenerate: { directory: 'basics' } }],
				},
			],
		}),
	],
	// Same headers as public/_headers, so the emulator also runs in dev and preview.
	server: {
		headers: {
			'Cross-Origin-Opener-Policy': 'same-origin',
			'Cross-Origin-Embedder-Policy': 'require-corp',
		},
	},
	// Pages import snippet sources from ../snippets with ?raw.
	vite: { server: { fs: { allow: ['..'] } } },
});
