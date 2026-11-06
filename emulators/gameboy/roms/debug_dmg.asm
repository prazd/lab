INCLUDE "hardware.inc/hardware.inc"
INCLUDE "constants.inc"
INCLUDE "header.inc"


SECTION "Boot ROM", ROM0[$0000]

EntryPoint:
    ld sp, hStackBottom
    xor a
    ld hl, _VRAM + SIZEOF(VRAM) - 1
.clearVRAM
    ld [hld], a
    bit 7, h
    jr nz, .clearVRAM

    ld hl, rNR52
    ld c, LOW(rNR11) ; CH1 length
    ; Enable APU
    ; This sets (roughly) all audio registers to 0
    ld a, AUDENA_ON
    ld [hld], a
    ASSERT rNR52 - 1 == rNR51
    ; Set CH1 duty cycle to 25%
    ASSERT AUDENA_ON == AUDLEN_DUTY_50
    ldh [c], a
    inc c
    ASSERT LOW(rNR11) + 1 == LOW(rNR12)
    ; Set CH1 envelope
    ld a, (15 << 4) | AUDENV_DOWN | 3 ; Initial volume 15, decreasing sweep 3
    ldh [c], a
    ; Route all channels to left speaker, CH2 and CH1 to right speaker
    ; Note that only channel 1 will be used!
    ld [hld], a
    ASSERT rNR51 - 1 == rNR50
    ; Set volume on both speakers to max, disable VIN on both speakers
    ld a, $77
    ld [hl], a

    ld a, %11_11_11_00
    ldh [rBGP], a

    ld de, HeaderLogo
    ld hl, vLogoTiles
.decompressLogo
    ld a, [de]
    call DecompressFirstNibble
    call DecompressSecondNibble
    inc de
    ld a, e
    cp LOW(HeaderTitle)
    jr nz, .decompressLogo


    ; ld hl, vRTile
    ld de, RTile
    ld b, 8
.copyRTile
    ld a, [de]
    inc de
    ld [hli], a
    inc hl ; Skip every other byte
    dec b
    jr nz, .copyRTile
    ld a, $19
    ld [vMainTilemap + SCRN_VX_B * 8 + 16], a ; 8 rows down, 16 across 0x9910

    ld hl, vMainTilemap + SCRN_VX_B * 9 + 15 ; 0x992f
.writeTilemapRow
    ld c, 12
.writeTilemapByte
    dec a
    jr z, ScrollLogo
    ld [hld], a
    dec c
    jr nz, .writeTilemapByte
    ld l, LOW(vMainTilemap + SCRN_VX_B * 8 + 15)
    jr .writeTilemapRow


ScrollLogo:
    jp End


DecompressFirstNibble:
    ld c, a
DecompressSecondNibble:
    ld b, 8 / 2 ; Set all 8 bits of a, "consuming" 4 bits of c
.loop
    push bc
    rl c ; Extract MSB of c
    rla ; Into LSB of a
    pop bc
    rl c ; Extract that same bit
    rla ; So that bit is inserted twice in a (= horizontally doubled)
    dec b
    jr nz, .loop
    ld [hli], a
    inc hl ; Skip second plane
    ld [hli], a ; Also double vertically
    inc hl
    ret


; Whitespace is not stripped after line continuations until RGBDS v0.6.0, so rows are not indented
Logo:
    logo_row_gfx_nybbles \
XX.., .XX., XX.., ...., ...., ...., ...., ...., ...., ...X, X..., ...., \
XXX., .XX., XX.., ...., ..XX, ...., ...., ...., ...., ...X, X..., ...., \
XXX., .XX., ...., ...., .XXX, X..., ...., ...., ...., ...X, X..., ...., \
XX.X, .XX., XX.X, X.XX, ..XX, ..XX, XX.., XX.X, X..., XXXX, X..X, XXX.
    logo_row_gfx_nybbles \
XX.X, .XX., XX.X, XX.X, X.XX, .XX., .XX., XXX., XX.X, X..X, X.XX, ..XX, \
XX.., XXX., XX.X, X..X, X.XX, .XXX, XXX., XX.., XX.X, X..X, X.XX, ..XX, \
XX.., XXX., XX.X, X..X, X.XX, .XX., ...., XX.., XX.X, X..X, X.XX, ..XX, \
XX.., .XX., XX.X, X..X, X.XX, ..XX, XXX., XX.., XX.., XXXX, X..X, XXX.


RTile:
    PUSHO
    OPT b.X
    db %..XXXX..
    db %.X....X.
    db %X.XXX..X
    db %X.X..X.X
    db %X.XXX..X
    db %X.X..X.X
    db %.X....X.
    db %..XXXX..
    POPO


End:
    ld a, LCDCF_ON | LCDCF_BLK01 | LCDCF_BGON
    ldh [rLCDC], a
    ld a, 1
    ldh [rBANK], a
    ;ASSERT @ == $100 ; Execution now falls through to the cartridge's header



SECTION "VRAM tiles", VRAM[_VRAM], BANK[0]

vBlankTile:
    ds TILE_SIZE
vLogoTiles:
    ds (HeaderTitle - HeaderLogo) * TILE_SIZE / 2
vRTile:
    ds TILE_SIZE

SECTION "VRAM tilemap", VRAM[_SCRN0], BANK[0]

vMainTilemap:
    ds SCRN_VX_B * SCRN_VY_B


SECTION "HRAM", HRAM[$FFEE]

    ds $10
hStackBottom:
