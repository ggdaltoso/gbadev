// The kinds of game a page suits best. A page lists them in its `genres` frontmatter;
// a page without it suits any genre. Each genre is a white glyph on a colored square.

const glyphs = {
	shooter: '<circle cx="8" cy="8" r="3.5"/><path d="M8 2v3M8 11v3M2 8h3M11 8h3"/>',
	platformer: '<path d="M2.5 13.5h4v-3h3v-3h4"/><path d="M3.5 8.5Q5 3 9 4.5"/>',
	'top-down': '<circle cx="8" cy="8" r="5.5"/><path d="M8 4l2 4-2 4-2-4z" fill="#fff"/>',
	runner: '<path d="M3.5 4l4 4-4 4M8.5 4l4 4-4 4"/>',
	puzzle: '<path d="M3 3h10v10H3zM6.3 3v10M9.7 3v10M3 6.3h10M3 9.7h10"/>',
};

export type GenreId = keyof typeof glyphs;

export const genres: { id: GenreId; name: string; color: string; about: string }[] = [
	{ id: 'shooter', name: 'Shooter', color: '#ff4d8d', about: 'Shots and waves of enemies, like Space Invaders.' },
	{ id: 'platformer', name: 'Platformer', color: '#2f9e36', about: 'Seen from the side, with gravity and jumps.' },
	{ id: 'top-down', name: 'Top-down', color: '#1e7cb8', about: 'Adventure and RPG seen from above: maps, talking, exploring.' },
	{ id: 'runner', name: 'Runner', color: '#d9650b', about: 'Always moving forward, dodging what comes.' },
	{ id: 'puzzle', name: 'Puzzle and grid', color: '#7c4dcc', about: 'Things locked to cells: blocks, bombs, sliding pieces.' },
];

export const genreIds = genres.map((genre) => genre.id) as [GenreId, ...GenreId[]];

export const genre = (id: GenreId) => genres.find((g) => g.id === id)!;

export const genreSvg = (id: GenreId) =>
	`<svg xmlns="http://www.w3.org/2000/svg" viewBox="0 0 16 16"><rect width="16" height="16" rx="3.5" fill="${genre(id).color}"/>` +
	`<g fill="none" stroke="#fff" stroke-width="1.5" stroke-linecap="round" stroke-linejoin="round">${glyphs[id]}</g></svg>`;

export const genreUrl = (id: GenreId) => `url("data:image/svg+xml,${encodeURIComponent(genreSvg(id))}")`;
