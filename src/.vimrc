set noet
set ts=8 sw=4
set sts=0

finish
" add bottom lines to your .vimrc
" curdir vimrc ===================================== 
fun! s:CurDirVIMRC()
  if getcwd() == expand("~") 
    return 
  endif
  for f in [ '.vimrc' , '_vimrc' ]
    if filereadable(f) 
      redraw
      exec 'silent so '.f
      echo "Found .vimrc in current directory. Loaded"
      return
    endif
  endfor
endf
com! LoadCurDirVimRC  :cal s:CurDirVIMRC()
autocmd VimEnter * :LoadCurDirVimRC

