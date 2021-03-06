-- Computernight Level

function level_size()
    return 40, 28
end

function level_koth_pos()
    return 20, 14
end

function level_init()
    world_fill_all(TILE_GFX_SNOW_SOLID)

    world_dig(20, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(20, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(20, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(20, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(37, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(38, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(38, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(37, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(37, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 7, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 5, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(29, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(30, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(31, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(32, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 25, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(4, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(4, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(5, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(6, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(8, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 22, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(9, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(10, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(11, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(12, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(38, 8, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(37, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(36, 4, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 3, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(34, 3, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(33, 3, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(20, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(26, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(27, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(28, 24, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 23, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(7, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(13, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(14, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(15, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(16, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(17, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(18, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(19, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(20, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(21, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(22, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(23, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(24, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(25, 6, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 9, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 10, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 11, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 12, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 13, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 14, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 15, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 16, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 17, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 18, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 19, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 20, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)
    world_dig(35, 21, TILE_PLAIN, TILE_GFX_SNOW_PLAIN)

    world_make_border(TILE_GFX_SNOW_BORDER)

    food_spawner = {}
    for s = 0, 10 do
        local dx, dy = world_find_digged()
        food_spawner[s] = { x = dx,
                            y = dy,
                            r = math.random(3),
                            a = math.random(100) + 30,
                            i = math.random(1000) + 1000,
                            n = game_time() }
        world_add_food(food_spawner[s].x, 
                       food_spawner[s].y, 
                       10000)
    end

    last_food = game_time()
end

function level_tick()
    if game_time() > last_food + 10000 then
        for n, spawner in pairs(food_spawner) do
            if game_time() > spawner.n then 
                world_add_food(spawner.x + math.random(spawner.r * 2 + 1) - spawner.r,
                               spawner.y + math.random(spawner.r * 2 + 1) - spawner.r,
                               spawner.a)
                spawner.n = spawner.n + spawner.i                               
            end
        end
    end
end
