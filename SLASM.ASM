; Define this to prefer correct char palette over sprite palette
;GETCHARPALETTE EQU 0
; Define this for Mode X to use a perfect sized resolution (256x240)
;FINESIZE EQU 0
; Define this to offset sprites by the change in scroll coords this frame.

IFDEF GETCHARPALETTE
SPRITETRANSPARENTTHRESHOLD  EQU 188
ELSE
SPRITETRANSPARENTTHRESHOLD  EQU 248
ENDIF

  .486p
  .MODEL FLAT

  ; The memory!
  EXTRN        _rom:DWORD
  EXTRN        _ram:DWORD
  EXTRN        _music:DWORD
  EXTRN        _sprites:DWORD
  EXTRN        _chars:DWORD
  EXTRN        _tiles:DWORD
  EXTRN        _lfb:DWORD
  EXTRN        _scroll1vscreen:DWORD
  EXTRN        _scroll2vscreen:DWORD
  EXTRN        _scroll1vscreenlastline:DWORD
  EXTRN        _scroll2vscreenlastline:DWORD
  EXTRN        _vscreen:DWORD
  EXTRN        _BCPaletteAddress:DWORD
  EXTRN        _vramvscreen:DWORD
  EXTRN        _scroll1xpos:DWORD
  EXTRN        _scroll1ypos:DWORD
  EXTRN        _scroll2xpos:DWORD
  EXTRN        _scroll2ypos:DWORD
  EXTRN        _newgfxaddress:DWORD
  EXTRN        _screenwidth:DWORD
  EXTRN        _screenheight:DWORD
  EXTRN        _foregroundscroll2:BYTE

  PUBLIC _buildscreen_
  PUBLIC _showscreenvesa2l_
  PUBLIC _showscreenmodex_
  PUBLIC _buildpalette_
  PUBLIC _newchartile_
  PUBLIC _newscroll1tile_
  PUBLIC _newscroll2tile_
  PUBLIC _buildpalette_

  .STACK 200h     ; Half a K of stack!

  .DATA           ; The DATA segment.

  .CODE           ; The CODE segment.

intensityscale:
  inc   al
  push  ax
  mul   bl        ; AX=AL*BL
  mov   bl,5
  div   bl        ; AL=AX/BL
  mov   bl,al
  pop   ax
  push  ax
  mul   cl        ; AX=AL*CL
  mov   cl,5
  div   cl        ; AL=AX/CL
  mov   cl,al
  pop   ax
  mul   dl        ; AX=AL*BL
  mov   dl,5
  div   dl        ; AL=AX/BL
  mov   dl,al
  ret

_buildpalette_:
  pushad
  mov   edi,[_ram]
  mov   esi,[_BCPaletteAddress]
  add   edi,00018000h
  mov   eax,4
buildpaletteouterloop:
  push  eax
  mov   eax,0
buildpaletteloop:
  add   esi,eax
  push  eax
  mov   ax,[edi]
  mov   bx,ax
  mov   cx,bx
  mov   dx,cx
  and   bx,0F000h
  shr   bx,10
  and   cx,00F00h
  shr   cx,6
  and   dx,000F0h
  shr   dx,2
  and   ax,0000Fh
  cmp   ax,0000Fh
  je    nointensityscaling1
  call  intensityscale
nointensityscaling1:
  mov   [esi+(64*4)],bl     ; SCROLL2 colours (64)
  mov   [esi+1+(64*4)],cl
  mov   [esi+2+(64*4)],dl
  mov   ax,[edi+200h]
  mov   bx,ax
  mov   cx,bx
  mov   dx,cx
  and   bx,0F000h
  shr   bx,10
  and   cx,00F00h
  shr   cx,6
  and   dx,000F0h
  shr   dx,2
  and   ax,0000Fh
  cmp   ax,0000Fh
  je    nointensityscaling2
  call  intensityscale
nointensityscaling2:
  mov   [esi],bl            ; SCROLL 1 colours (64)
  mov   [esi+1],cl
  mov   [esi+2],dl
IFDEF GETCHARPALETTE
  mov   ax,[edi+400h]
  mov   bx,ax
  mov   cx,bx
  mov   dx,cx
  and   bx,0F000h
  shr   bx,10
  and   cx,00F00h
  shr   cx,6
  and   dx,000F0h
  shr   dx,2
  and   ax,0000Fh
  cmp   ax,0000Fh
  je    nointensityscaling3
  call  intensityscale
nointensityscaling3:
  mov   [esi+(128*4)],bl         ; Sprite colours (64)
  mov   [esi+(128*4)+1],cl       ; Only done if were getting char palette.
  mov   [esi+(128*4)+2],dl
ENDIF
  add   edi,2     ; next palette entry
  pop   eax
  sub   esi,eax
  add   eax,16
  cmp   eax,240
  jl    buildpaletteloop
  add   esi,4
  add   edi,2
  pop   eax
  dec   eax
  jnz   buildpaletteouterloop
IFDEF GETCHARPALETTE
buildcharpalette:   ; need to do this differently
  mov   edi,[_ram]
  mov   esi,[_BCPaletteAddress]
  add   esi,192*4
  add   edi,00018600h
  mov   al,16
charpaletteouterloop:
  mov   ah,4
charpaletteloop:
  push  eax
  mov   ax,[edi]
  mov   bx,ax
  mov   cx,bx
  mov   dx,cx
  and   bx,0F000h
  shr   bx,10
  and   cx,00F00h
  shr   cx,6
  and   dx,000F0h
  shr   dx,2
  and   ax,0000Fh
  cmp   ax,0000Fh
  je    nointensityscaling4
  call  intensityscale
nointensityscaling4:
  mov   [esi],bl               ; CHAR colours (64)
  mov   [esi+1],cl
  mov   [esi+2],dl
  add   esi,16*4    ; skip 16 colours to get to the next one
  add   edi,2
  pop   eax
  dec   ah
  jnz   charpaletteloop
  sub   esi,(16*16)-4   ; back to where we started, but one entry on.
  dec   al
  jnz   charpaletteouterloop
ELSE
buildspritepalette:
  mov   edi,[_ram]
  mov   esi,[_BCPaletteAddress]
  add   esi,128*4
  add   edi,00018400h
  mov   al,8
spritepaletteouterloop:
  mov   ah,16
spritepaletteloop:
  push  eax
  mov   ax,[edi]
  mov   bx,ax
  mov   cx,bx
  mov   dx,cx
  and   bx,0F000h
  shr   bx,10
  and   cx,00F00h
  shr   cx,6
  and   dx,000F0h
  shr   dx,2
  and   ax,0000Fh
  cmp   ax,0000Fh
  je    nointensityscaling5
  call  intensityscale
nointensityscaling5:
  mov   [esi],bl             ; SPRITE colours (128)
  mov   [esi+1],cl           ; Only done if we're not getting char colours.
  mov   [esi+2],dl
  add   esi,8*4    ; skip 8 colours to get to the next one
  add   edi,2
  pop   eax
  dec   ah
  jnz   spritepaletteloop
  sub   esi,(32*16)-4   ; back to where we started, but one entry on.
  dec   al
  jnz   spritepaletteouterloop
ENDIF
  popad
  ret

_newchartile_:
  pushad
  mov   edi,[_newgfxaddress]
  mov   eax,edi
  sub   edi,0C000h
  mov   ebx,edi
  and   edi,0FFFFFFC0h      ; 64 bytes per row of char data
  shl   edi,5               ; but 2048 bytes per row of char image data.
  and   ebx,00000003Fh      ; 0-63 is the X offset (2 bytes per char)
  shl   ebx,2               ; But onscreen, each char is 8 bytes long.
  add   edi,ebx
  add   edi,[_vramvscreen]
  mov   esi,[_chars]
  add   eax,[_ram]
  movzx edx,word ptr [eax]
  and   edx,000000FFh
  mov   ax,word ptr [eax+0800h]
  mov   bx,ax
  and   ebx,000000F0h
  shl   ebx,2
  add   edx,ebx
  shl   edx,6
  add   esi,edx
  and   ax,000Fh
  mov   ah,al
  shl   eax,8
  mov   al,ah
  shl   eax,8
  mov   al,ah
  mov   cl,8
drawchartileloop:
  mov   ebx,[esi]
  add   ebx,eax
  mov   [edi],ebx
  mov   ebx,[esi+4]
  add   ebx,eax
  mov   [edi+4],ebx
  add   edi,256
  add   esi,8
  dec   cl
  jnz   drawchartileloop
  popad
  ret

_newscroll1tile_:
  pushad
  mov   edi,[_newgfxaddress]
  and   edi,0FFFFFFFCh
  mov   eax,edi
  sub   edi,10000h
  mov   ebx,edi
  and   edi,0FFFFFF00h
  shl   edi,7             ; each row of tiles is 32768 bytes
  and   ebx,0000000FFh
  shl   ebx,2
  add   edi,ebx
  add   edi,[_scroll1vscreen]
  mov   esi,[_tiles]
  add   eax,[_ram]
  mov   edx,[eax]
  mov   ebx,edx
  mov   eax,edx
  and   eax,00180000h
  shr   eax,11
  mov   al,ah
  shl   eax,8
  mov   al,ah
  shl   eax,8
  mov   al,ah
  shl   dx,8
  and   edx,07FF00h
  add   esi,edx
  shr   ebx,16
  mov   cl,8
  bt    bx,6
  jc    ns1verticallyflipped
  bt    bx,7
  jc    ns1hfpixel_loop
ns1pixel_loop:
  mov   ebx,[esi]
  or    ebx,eax
  mov   [edi],ebx
  mov   [edi+1024],ebx
  mov   ebx,[esi+4]
  or    ebx,eax
  mov   [edi+4],ebx
  mov   [edi+4+1024],ebx
  mov   ebx,[esi+64]
  or    ebx,eax
  mov   [edi+(2048*8)],ebx
  mov   [edi+(2048*8)+1024],ebx
  mov   ebx,[esi+68]
  or    ebx,eax
  mov   [edi+((2048*8)+4)],ebx
  mov   [edi+((2048*8)+4)+1024],ebx
  mov   ebx,[esi+128]
  or    ebx,eax
  mov   [edi+8],ebx
  mov   [edi+8+1024],ebx
  mov   ebx,[esi+132]
  or    ebx,eax
  mov   [edi+12],ebx
  mov   [edi+12+1024],ebx
  mov   ebx,[esi+192]
  or    ebx,eax
  mov   [edi+((2048*8)+8)],ebx
  mov   [edi+((2048*8)+8)+1024],ebx
  mov   ebx,[esi+196]
  or    ebx,eax
  mov   [edi+((2048*8)+12)],ebx
  mov   [edi+((2048*8)+12)+1024],ebx
  add   esi,8
  add   ebx,eax
  add   edi,2048
  dec   cl
  jnz   ns1pixel_loop
  popad
  ret
ns1hfpixel_loop:
  mov   bl,[esi+135]
  or    bl,al
  mov   [edi],bl
  mov   [edi+1024],bl
  mov   bl,[esi+134]
  or    bl,al
  mov   [edi+1],bl
  mov   [edi+1+1024],bl
  mov   bl,[esi+133]
  or    bl,al
  mov   [edi+2],bl
  mov   [edi+2+1024],bl
  mov   bl,[esi+132]
  or    bl,al
  mov   [edi+3],bl
  mov   [edi+3+1024],bl
  mov   bl,[esi+131]
  or    bl,al
  mov   [edi+4],bl
  mov   [edi+4+1024],bl
  mov   bl,[esi+130]
  or    bl,al
  mov   [edi+5],bl
  mov   [edi+5+1024],bl
  mov   bl,[esi+129]
  or    bl,al
  mov   [edi+6],bl
  mov   [edi+6+1024],bl
  mov   bl,[esi+128]
  or    bl,al
  mov   [edi+7],bl
  mov   [edi+7+1024],bl
  mov   bl,[esi+7]
  or    bl,al
  mov   [edi+8],bl
  mov   [edi+8+1024],bl
  mov   bl,[esi+6]
  or    bl,al
  mov   [edi+9],bl
  mov   [edi+9+1024],bl
  mov   bl,[esi+5]
  or    bl,al
  mov   [edi+10],bl
  mov   [edi+10+1024],bl
  mov   bl,[esi+4]
  or    bl,al
  mov   [edi+11],bl
  mov   [edi+11+1024],bl
  mov   bl,[esi+3]
  or    bl,al
  mov   [edi+12],bl
  mov   [edi+12+1024],bl
  mov   bl,[esi+2]
  or    bl,al
  mov   [edi+13],bl
  mov   [edi+13+1024],bl
  mov   bl,[esi+1]
  or    bl,al
  mov   [edi+14],bl
  mov   [edi+14+1024],bl
  mov   bl,[esi]
  or    bl,al
  mov   [edi+15],bl
  mov   [edi+15+1024],bl
  mov   bl,[esi+199]
  or    bl,al
  mov   [edi+((2048*8))],bl
  mov   [edi+((2048*8))+1024],bl
  mov   bl,[esi+198]
  or    bl,al
  mov   [edi+((2048*8)+1)],bl
  mov   [edi+((2048*8)+1)+1024],bl
  mov   bl,[esi+197]
  or    bl,al
  mov   [edi+((2048*8)+2)],bl
  mov   [edi+((2048*8)+2)+1024],bl
  mov   bl,[esi+196]
  or    bl,al
  mov   [edi+((2048*8)+3)],bl
  mov   [edi+((2048*8)+3)+1024],bl
  mov   bl,[esi+195]
  or    bl,al
  mov   [edi+((2048*8)+4)],bl
  mov   [edi+((2048*8)+4)+1024],bl
  mov   bl,[esi+194]
  or    bl,al
  mov   [edi+((2048*8)+5)],bl
  mov   [edi+((2048*8)+5)+1024],bl
  mov   bl,[esi+193]
  or    bl,al
  mov   [edi+((2048*8)+6)],bl
  mov   [edi+((2048*8)+6)+1024],bl
  mov   bl,[esi+192]
  or    bl,al
  mov   [edi+((2048*8)+7)],bl
  mov   [edi+((2048*8)+7)+1024],bl
  mov   bl,[esi+71]
  or    bl,al
  mov   [edi+((2048*8)+8)],bl
  mov   [edi+((2048*8)+8)+1024],bl
  mov   bl,[esi+70]
  or    bl,al
  mov   [edi+((2048*8)+9)],bl
  mov   [edi+((2048*8)+9)+1024],bl
  mov   bl,[esi+69]
  or    bl,al
  mov   [edi+((2048*8)+10)],bl
  mov   [edi+((2048*8)+10)+1024],bl
  mov   bl,[esi+68]
  or    bl,al
  mov   [edi+((2048*8)+11)],bl
  mov   [edi+((2048*8)+11)+1024],bl
  mov   bl,[esi+67]
  or    bl,al
  mov   [edi+((2048*8)+12)],bl
  mov   [edi+((2048*8)+12)+1024],bl
  mov   bl,[esi+66]
  or    bl,al
  mov   [edi+((2048*8)+13)],bl
  mov   [edi+((2048*8)+13)+1024],bl
  mov   bl,[esi+65]
  or    bl,al
  mov   [edi+((2048*8)+14)],bl
  mov   [edi+((2048*8)+14)+1024],bl
  mov   bl,[esi+64]
  or    bl,al
  mov   [edi+((2048*8)+15)],bl
  mov   [edi+((2048*8)+15)+1024],bl
  add   esi,8
  add   edi,2048
  dec   cl
  jnz   ns1hfpixel_loop
  popad
  ret
ns1verticallyflipped:
  bt    bx,7
  jc    ns1pixel_loop
  add   edi,(2048*16)
ns1vfpixel_loop:
  sub   edi,2048
  mov   ebx,[esi]
  or    ebx,eax
  mov   [edi],ebx
  mov   [edi+1024],ebx
  mov   ebx,[esi+4]
  or    ebx,eax
  mov   [edi+4],ebx
  mov   [edi+4+1024],ebx
  mov   ebx,[esi+64]
  or    ebx,eax
  mov   [edi-(2048*8)],ebx
  mov   [edi-((2048*8)-1024)],ebx
  mov   ebx,[esi+68]
  or    ebx,eax
  mov   [edi-((2048*8)-4)],ebx
  mov   [edi-(((2048*8)-4)-1024)],ebx
  mov   ebx,[esi+128]
  or    ebx,eax
  mov   [edi+8],ebx
  mov   [edi+8+1024],ebx
  mov   ebx,[esi+132]
  or    ebx,eax
  mov   [edi+12],ebx
  mov   [edi+12+1024],ebx
  mov   ebx,[esi+192]
  or    ebx,eax
  mov   [edi-((2048*8)-8)],ebx
  mov   [edi-(((2048*8)-8)-1024)],ebx
  mov   ebx,[esi+196]
  or    ebx,eax
  mov   [edi-((2048*8)-12)],ebx
  mov   [edi-(((2048*8)-12)-1024)],ebx
  add   esi,8
  dec   cl
  jnz   ns1vfpixel_loop
newscroll1done:
  popad
  ret

_newscroll2tile_:
  pushad
  mov   edi,[_newgfxaddress]
  and   edi,0FFFFFFFCh
  mov   eax,edi
  sub   edi,14000h
  mov   ebx,edi
  and   edi,0FFFFFF00h
  shl   edi,5        
  and   ebx,0000000FFh
  shl   ebx,1
  add   edi,ebx
  add   edi,[_scroll2vscreen]
  mov   esi,[_tiles]
  add   esi,080000h     ; using the scroll2 graphics bank.
  add   eax,[_ram]
  mov   edx,[eax]
  mov   ebx,edx
  mov   eax,edx
  and   eax,00180000h
  shr   eax,11
  mov   al,ah
  shl   eax,8
  mov   al,ah
  shl   eax,8
  mov   al,ah
  shr   ebx,16
  shl   dx,8
  and   edx,07FF00h
  shr   edx,2
  add   esi,edx
  mov   cl,8
  bt    bx,7
  jc    ns2hfpixel_yloop
ns2pixel_yloop:
  mov   ebx,[esi]
  or    ebx,eax
  mov   [edi],ebx
  mov   [edi+512],ebx
  mov   ebx,[esi+4]
  or    ebx,eax
  mov   [edi+4],ebx
  mov   [edi+4+512],ebx
  add   edi,1024
  add   esi,8
  dec   cl
  jnz   ns2pixel_yloop
  popad
  ret
ns2hfpixel_yloop:
  mov   bl,[esi+7]
  or    bl,al
  mov   [edi],bl
  mov   [edi+512],bl
  mov   bl,[esi+6]
  or    bl,al
  mov   [edi+1],bl
  mov   [edi+1+512],bl
  mov   bl,[esi+5]
  or    bl,al
  mov   [edi+2],bl
  mov   [edi+2+512],bl
  mov   bl,[esi+4]
  or    bl,al
  mov   [edi+3],bl
  mov   [edi+3+512],bl
  mov   bl,[esi+3]
  or    bl,al
  mov   [edi+4],bl
  mov   [edi+4+512],bl
  mov   bl,[esi+2]
  or    bl,al
  mov   [edi+5],bl
  mov   [edi+5+512],bl
  mov   bl,[esi+1]
  or    bl,al
  mov   [edi+6],bl
  mov   [edi+6+512],bl
  mov   bl,[esi]
  or    bl,al
  mov   [edi+7],bl
  mov   [edi+7+512],bl
  add   edi,1024
  add   esi,8
  dec   cl
  jnz   ns2hfpixel_yloop
  popad
  ret

_buildscreen_:
; This procedure copies all graphics to the screen.
  pushad
  mov   edi,[_ram]
  mov   esi,edi
  add   esi,14002h  ; into SCROLL2 memory
  mov   al,[esi]
  and   al,1
  mov   [_foregroundscroll2],al
  add   edi,008010h
  movzx eax,byte ptr [edi+0]
  mov   ah,[edi+1]
  movzx ebx,byte ptr [edi+2]
  mov   bh,[edi+3]
  movzx ecx,byte ptr [edi+4]
  mov   ch,[edi+5]
  movzx edx,byte ptr [edi+6]
  mov   dh,[edi+7]
  movsx eax,ax
  movsx ebx,bx
  movsx ecx,cx
  movsx edx,dx
  and   eax,1023
  and   ebx,1023
  and   ecx,511
  mov   [_scroll1xpos],eax
  mov   [_scroll1ypos],ebx
  mov   [_scroll2xpos],ecx
blitscreen:
; This part blits all three layers into the main virtual screen, taking
; transparent colours into account.
  cmp   [_foregroundscroll2],0
  jne   blitscroll1notransparency



blitscroll2notransparency:
; This is for when SCROLL2 is the background. No transparency required.
  cmp   edx,256
  jle   noscroll2ymaxoverflow
  mov   edx,256
noscroll2ymaxoverflow:
  cmp   edx,256
  jbe   noscroll2yminoverflow
  xor   edx,edx
noscroll2yminoverflow:
  mov   [_scroll2ypos],edx
  mov   esi,[_vscreen]
  add   esi,(288*16)+16   ; no need for the spare sprite-buffer area yet
  mov   edi,[_scroll2ypos]
  shl   edi,10
  add   edi,[_scroll2xpos]
  add   edi,[_scroll2vscreen]
  sub   edi,768
  mov   dx,240
blitscroll2ntyloop:
  add   edi,768
  mov   cx,16
blitscroll2ntxloop:
  mov   ebx,[edi]
  mov   [esi],ebx
  mov   ebx,[edi+4]
  mov   [esi+4],ebx
  mov   ebx,[edi+8]
  mov   [esi+8],ebx
  mov   ebx,[edi+12]
  mov   [esi+12],ebx
  add   edi,16
  add   esi,16
  dec   cx
  jnz   blitscroll2ntxloop
  add   esi,32      ; Skip left and right sprite-buffer areas
  dec   dx
  jnz   blitscroll2ntyloop
blitscroll1transparency:
; SCROLL1 is in front. Must see SCROLL2 through transparent areas.
  mov   esi,[_vscreen]
  add   esi,(288*16)+16   ; no need for the spare sprite-buffer area yet
  mov   edi,[_scroll1ypos]
  shl   edi,11
  add   edi,[_scroll1xpos]
  add   edi,[_scroll1vscreen]
  sub   edi,768+1024
  mov   dx,240
blitscroll1tyloop:
  add   edi,768+1024
  mov   cx,256
blitscroll1txloop:
  mov   bl,[edi]
  cmp   bl,60
  jge   blitscroll1ttransparent
  mov   [esi],bl
blitscroll1ttransparent:
  inc   edi
  inc   esi
  dec   cx
  jnz   blitscroll1txloop
  cmp   edi,[_scroll1vscreenlastline]
  jb    noblitscroll1tywrap
  sub   edi,2048*1024
noblitscroll1tywrap:
  add   esi,32      ; Skip left and right sprite-buffer areas
  dec   dx
  jnz   blitscroll1tyloop
  jmp   showsprites




blitscroll1notransparency:
; SCROLL2 is in front. Must see SCROLL1 through transparent areas.
  and   edx,511
  mov   [_scroll2ypos],edx
  mov   esi,[_vscreen]
  add   esi,(288*16)+16   ; no need for the spare sprite-buffer area yet
  mov   edi,[_scroll1ypos]
  shl   edi,11
  add   edi,[_scroll1xpos]
  add   edi,[_scroll1vscreen]
  sub   edi,768+1024
  mov   dx,240
blitscroll1ntyloop:
  add   edi,768+1024
  mov   cx,16
blitscroll1ntxloop:
  mov   ebx,[edi]
  mov   [esi],ebx
  mov   ebx,[edi+4]
  mov   [esi+4],ebx
  mov   ebx,[edi+8]
  mov   [esi+8],ebx
  mov   ebx,[edi+12]
  mov   [esi+12],ebx
  add   edi,16
  add   esi,16
  dec   cx
  jnz   blitscroll1ntxloop
  cmp   edi,[_scroll1vscreenlastline]
  jb    noblitscroll1ntywrap
  sub   edi,2048*1024
noblitscroll1ntywrap:
  add   esi,32      ; Skip left and right sprite-buffer areas
  dec   dx
  jnz   blitscroll1ntyloop
blitscroll2transparency:
; This is for when SCROLL2 is in the foreground. Transparency required.
  mov   esi,[_vscreen]
  add   esi,(288*16)+16   ; no need for the spare sprite-buffer area yet
  mov   edi,[_scroll2ypos]
  shl   edi,10
  add   edi,[_scroll2xpos]
  add   edi,[_scroll2vscreen]
  sub   edi,768
  mov   dx,240
blitscroll2tyloop:
  add   edi,768
  mov   cx,256
blitscroll2txloop:
  mov   bl,[edi]
  cmp   bl,124
  jge   blitscroll2ttransparent
  mov   [esi],bl
blitscroll2ttransparent:
  inc   edi
  inc   esi
  dec   cx
  jnz   blitscroll2txloop
  cmp   edi,[_scroll2vscreenlastline]
  jb    noblitscroll2tywrap
  sub   edi,1024*512
noblitscroll2tywrap:
  add   esi,32      ; Skip left and right sprite-buffer areas
  dec   dx
  jnz   blitscroll2tyloop


showsprites:
; This draws the sprites directly onto the vscreen. Since sprites don't have
; a section of tiled memory, we can't redraw it each time without clearing
; it beforehand, which takes time. Quicker to just draw 'em here.
  mov   edi,[_ram]
  add   edi,0CF8h
  mov   cl,160
spritetableloop:
  movzx edx,word ptr [edi]
  cmp   edx,07FFh
  je    finisheddrawingsprite
  shl   edx,8               
  movzx ebx,word ptr [edi+4]     ; y value goes into EBX
  cmp   ebx,512-16
  jle   notywrapped
  sub   ebx,512
notywrapped:
  cmp   ebx,240
  jge   finisheddrawingsprite
  add   ebx,16
  mov   esi,[_sprites]
  add   esi,edx             ; ESI now contains sprite image address
  mov   edx,ebx
  shl   edx,5
  shl   ebx,8       ; each vscreen row is 288 pixels (y*256 + y*32)
  add   ebx,edx
  movsx edx,word ptr [edi+6]   ; x value goes into EDX
  cmp   edx,-16
  jle   finisheddrawingsprite
  cmp   edx,256
  jge   finisheddrawingsprite
  add   edx,16
  add   ebx,edx
  add   ebx,[_vscreen]      ; EBX now contains the virtual screen address
                            ; to draw the image to.
  mov   ax,[edi+2]
  and   al,001Ch
  shr   al,2
  mov   ch,16
  bt    word ptr [edi+2],1
  jc    drawspritehorizontallyflipped
drawsprite:
  mov   dl,[esi+00]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump01
  or    dl,al
  mov   [ebx+00],dl
drawspritejump01:
  mov   dl,[esi+01]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump02
  or    dl,al
  mov   [ebx+01],dl
drawspritejump02:
  mov   dl,[esi+02]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump03
  or    dl,al
  mov   [ebx+02],dl
drawspritejump03:
  mov   dl,[esi+03]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump04
  or    dl,al
  mov   [ebx+03],dl
drawspritejump04:
  mov   dl,[esi+04]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump05
  or    dl,al
  mov   [ebx+04],dl
drawspritejump05:
  mov   dl,[esi+05]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump06
  or    dl,al
  mov   [ebx+05],dl
drawspritejump06:
  mov   dl,[esi+06]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump07
  or    dl,al
  mov   [ebx+06],dl
drawspritejump07:
  mov   dl,[esi+07]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump08
  or    dl,al
  mov   [ebx+07],dl
drawspritejump08:
  mov   dl,[esi+08]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump09
  or    dl,al
  mov   [ebx+08],dl
drawspritejump09:
  mov   dl,[esi+09]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump10
  or    dl,al
  mov   [ebx+09],dl
drawspritejump10:
  mov   dl,[esi+10]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump11
  or    dl,al
  mov   [ebx+10],dl
drawspritejump11:
  mov   dl,[esi+11]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump12
  or    dl,al
  mov   [ebx+11],dl
drawspritejump12:
  mov   dl,[esi+12]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump13
  or    dl,al
  mov   [ebx+12],dl
drawspritejump13:
  mov   dl,[esi+13]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump14
  or    dl,al          
  mov   [ebx+13],dl
drawspritejump14:
  mov   dl,[esi+14]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump15
  or    dl,al
  mov   [ebx+14],dl
drawspritejump15:
  mov   dl,[esi+15]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritejump16
  or    dl,al
  mov   [ebx+15],dl
drawspritejump16:
  add   ebx,288
  add   esi,16
  dec   ch
  jnz   drawsprite
finisheddrawingsprite:
  sub   edi,8
  dec   cl
  jnz   spritetableloop
  jmp   blitvram
drawspritehorizontallyflipped:
  mov   dl,[esi+00]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump01
  or    dl,al
  mov   [ebx+15],dl
drawspritehfjump01:
  mov   dl,[esi+01]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump02
  or    dl,al
  mov   [ebx+14],dl
drawspritehfjump02:
  mov   dl,[esi+02]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump03
  or    dl,al
  mov   [ebx+13],dl
drawspritehfjump03:
  mov   dl,[esi+03]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump04
  or    dl,al
  mov   [ebx+12],dl
drawspritehfjump04:
  mov   dl,[esi+04]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump05
  or    dl,al
  mov   [ebx+11],dl
drawspritehfjump05:
  mov   dl,[esi+05]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump06
  or    dl,al
  mov   [ebx+10],dl
drawspritehfjump06:
  mov   dl,[esi+06]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump07
  or    dl,al
  mov   [ebx+09],dl
drawspritehfjump07:
  mov   dl,[esi+07]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump08
  or    dl,al
  mov   [ebx+08],dl
drawspritehfjump08:
  mov   dl,[esi+08]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump09
  or    dl,al
  mov   [ebx+07],dl
drawspritehfjump09:
  mov   dl,[esi+09]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump10
  or    dl,al
  mov   [ebx+06],dl
drawspritehfjump10:
  mov   dl,[esi+10]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump11
  or    dl,al
  mov   [ebx+05],dl
drawspritehfjump11:
  mov   dl,[esi+11]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump12
  or    dl,al
  mov   [ebx+04],dl
drawspritehfjump12:
  mov   dl,[esi+12]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump13
  or    dl,al
  mov   [ebx+03],dl
drawspritehfjump13:
  mov   dl,[esi+13]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump14
  or    dl,al          
  mov   [ebx+02],dl
drawspritehfjump14:
  mov   dl,[esi+14]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump15
  or    dl,al
  mov   [ebx+01],dl
drawspritehfjump15:
  mov   dl,[esi+15]
  cmp   dl,SPRITETRANSPARENTTHRESHOLD
  jge   drawspritehfjump16
  or    dl,al
  mov   [ebx+00],dl
drawspritehfjump16:
  add   ebx,288
  add   esi,16
  dec   ch
  jnz   drawspritehorizontallyflipped
  sub   edi,8
  dec   cl
  jnz   spritetableloop




blitvram:
; Not so easy. LOADs of transparency.
  mov   esi,[_vscreen]
  add   esi,(288*16)+16   ; no need for the spare sprite-buffer area yet
  mov   edi,[_vramvscreen]
  mov   dx,240
blitvramyloop:
  mov   cx,256
blitvramxloop:
  mov   bl,[edi]
  cmp   bl,240
  jge   blitvramtransparent
  mov   [esi],bl
blitvramtransparent:
  inc   edi
  inc   esi
  dec   cx
  jnz   blitvramxloop
  add   esi,32      ; Skip left and right sprite-buffer areas
  dec   dx
  jnz   blitvramyloop
  popad
  ret

_showscreenvesa2l_:
  pushad
  mov   edi,[_screenwidth]
  sub   edi,256
  shr   edi,1
  add   edi,[_lfb]
  mov   esi,[_vscreen]
  add   esi,(288*16)+16     ; cut away the overlapping sprites
; This part copies the main virtual screen to video memory.
  mov   edx,[_screenheight]
  cmp   edx,240
  jle   blitscanlineloopvesa2l
  mov   edx,240
blitscanlineloopvesa2l:
  mov   ecx,16
blitendloopvesa2l:
  mov   ebx,[esi]
  mov   [edi],ebx
  mov   ebx,[esi+4]
  mov   [edi+4],ebx
  mov   ebx,[esi+8]
  mov   [edi+8],ebx
  mov   ebx,[esi+12]
  mov   [edi+12],ebx
  add   edi,16
  add   esi,16
  dec   ecx
  jnz   blitendloopvesa2l
  add   edi,[_screenwidth]
  sub   edi,256
  add   esi,32            ; skip the overlap areas.
  dec   edx
  jnz   blitscanlineloopvesa2l
  popad
  ret

_showscreenmodex_:
  pushad
;        mov     dx,3DAh           ; This is a WaitVBL() routine.
;WAIT1:                            ;
;        in      al,dx             ;
;        and     al,08h            ;
;        jnz     WAIT1             ;
;WAIT2:                            ;
;        in      al,dx             ;
;        and     al,08h            ;
;        jnz     WAIT2             ;
  mov   edi,[_vscreen]    ; The address of the 288x288 virtual screen.
  add   edi,(16*288)+19   ; EDI is at top left of image (16 pixel buffer on
                          ; all sides of vscreen).
  mov   ax,0802h          ; ModeX port thingy.
showscreenmodexportloop:
  mov   esi,[_lfb]        ; ESI is screen address.
IFNDEF FINESIZE
  add   esi,8             ; Centre 256x240 image in 320x240 screen.
ENDIF
  mov   dx,03C4h
  out   dx,ax             ; Select ModeX "bank" or something.
  mov   dx,240            ; 240 pixels high
showscreenmodexyloop:
  mov   cx,4
showscreenmodexxloop:
  mov   bl,[edi+60]       ; I've unrolled the loop a bit.
  shl   ebx,8
  mov   bl,[edi+56]
  shl   ebx,8
  mov   bl,[edi+52]
  shl   ebx,8
  mov   bl,[edi+48]
  mov   [esi+12],ebx
  mov   bl,[edi+44]       ; I've unrolled the loop a bit.
  shl   ebx,8
  mov   bl,[edi+40]
  shl   ebx,8
  mov   bl,[edi+36]
  shl   ebx,8
  mov   bl,[edi+32]
  mov   [esi+8],ebx
  mov   bl,[edi+28]       ; I've unrolled the loop a bit.
  shl   ebx,8
  mov   bl,[edi+24]
  shl   ebx,8
  mov   bl,[edi+20]
  shl   ebx,8
  mov   bl,[edi+16]
  mov   [esi+4],ebx
  mov   bl,[edi+12]
  shl   ebx,8
  mov   bl,[edi+8]
  shl   ebx,8
  mov   bl,[edi+4]
  shl   ebx,8
  mov   bl,[edi]
  mov   [esi],ebx
  add   edi,64
  add   esi,16
  dec   cx
  jnz   showscreenmodexxloop
  add   edi,32    ; skip the 16-pixel overlap areas at each side.
IFNDEF FINESIZE
  add   esi,16
ENDIF
  dec   dx
  jnz   showscreenmodexyloop
  sub   edi,(288*240)+1       ; Back to top left, but one pixel on.
  shr   ah,1                  ; Next ModeX "bank"
  jnz   showscreenmodexportloop
  popad
  ret

END
