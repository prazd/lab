SECTION "Header", ROM0[$100]
    nop
    jp EntryPoint

; Заголовок картриджа (заполняется для корректной работы)
    ds $150 - $104, 0

SECTION "Main", ROM0
EntryPoint:
    ; Отключение LCD перед инициализацией
    ld a, 0
    ld [$FF40], a

    ; Очистка видеопамяти (тайловой карты)
    ld hl, $9800 ; Начало тайловой карты
    ld bc, $9C00 - $9800 ; Размер карты
ClearMap:
    ld [hl], 0
    inc hl
    dec bc
    ld a, b
    or c
    jr nz, ClearMap

    ; Загрузка шрифта (простые тайлы для символов)
    ld hl, $8000 ; Начало тайловой памяти
    ld de, FontData
    ld bc, FontDataEnd - FontData
CopyFont:
    ld a, [de]
    ld [hl+], a
    inc de
    dec bc
    ld a, b
    or c
    jr nz, CopyFont

    ; Запись текста в тайловую карту
    ld hl, $9820 ; Позиция на экране (примерно центр)
    ld de, TextData
WriteText:
    ld a, [de]
    cp 0
    jr z, TextDone
    ld [hl+], a
    inc de
    jr WriteText
TextDone:

    ; Включение LCD
    ld a, %10010001 ; LCD on, BG on
    ld [$FF40], a

    ; Бесконечный цикл
Forever:
    jr Forever

; Данные шрифта (упрощённый пример для символа "A")
FontData:
    db $00, $00, $3C, $3C, $42, $42, $42, $42
    db $7E, $7E, $42, $42, $42, $42, $00, $00
FontDataEnd:

; Текст для отображения (индексы тайлов)
TextData:
    db 1, 0 ; "A" и конец строки (0 как терминатор)