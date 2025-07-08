SECTION "Header", ROM0[$100]
    nop
    jp Start

; Заголовок (минимально валидный)
    ds $150 - $104, 0

SECTION "Main", ROM0
Start:
    ; Отключение LCD (бит 7 в $FF40)
    ld a, [$FF40]
    res 7, a
    ld [$FF40], a

    ; Загрузка тайлов (буквы G, a, m, e, B, o, y)
    ld hl, TileData
    ld de, $8000       ; Адрес начала тайл-памяти
    ld bc, TileDataEnd - TileData
.copyTiles:
    ld a, [hl+]
    ld [de], a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, .copyTiles

    ; Запись индексов тайлов в BG map
    ld hl, $9800       ; Начало background map
    ld de, TileMapData
    ld bc, TileMapDataEnd - TileMapData
.copyMap:
    ld a, [de]
    ld [hl+], a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, .copyMap

    ; Включение LCD
    ld a, $91          ; LCD on, BG on, TileMap at $9800, TileData at $8000
    ld [$FF40], a

.loop:
    jr .loop           ; Бесконечный цикл

; ========================
; Данные тайлов (каждый 8x8, 16 байт)
; Можно заменить на реальные глифы
TileData:
; Символ G
    db %00011110
    db %00100001
    db %01000000
    db %01001111
    db %01000001
    db %00100001
    db %00011110
    db 0
; Символ a
    db %00000000
    db %00011100
    db %00100010
    db %00111110
    db %00100010
    db %00100010
    db %00111111
    db 0
; Символ m
    db %00000000
    db %01110110
    db %01001001
    db %01001001
    db %01001001
    db %01001001
    db %01001001
    db 0
; Символ e
    db %00000000
    db %00111100
    db %01000010
    db %01111110
    db %01000000
    db %01000010
    db %00111100
    db 0
; Символ B
    db %01111100
    db %01000010
    db %01000010
    db %01111100
    db %01000010
    db %01000010
    db %01111100
    db 0
; Символ o
    db %00000000
    db %00111100
    db %01000010
    db %01000010
    db %01000010
    db %01000010
    db %00111100
    db 0
; Символ y
    db %00000000
    db %01000010
    db %01000010
    db %01000010
    db %00111110
    db %00000010
    db %00111100
    db 0
TileDataEnd:

; Индексы в tilemap: G, a, m, e, B, o, y (начиная с тайла 0)
TileMapData:
    db 0, 1, 2, 3, 4, 5, 6
TileMapDataEnd:
