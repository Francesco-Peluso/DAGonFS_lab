add_custom_command(OUTPUT DAGonFS_P2P.ps
                   COMMAND groff -e -T ps -man ${CMAKE_CURRENT_SOURCE_DIR}/DAGonFS_P2P.man > DAGonFS_P2P.ps
                   DEPENDS DAGonFS_P2P.man
                   COMMENT "Creating postscript file")
add_custom_target(postscript ALL
                  DEPENDS DAGonFS_P2P.ps)

# Set GROFF_NO_SGR because the following txt rule running ul is incompatible with the 
# default.
add_custom_command(OUTPUT DAGonFS_P2P.1
                   COMMAND export GROFF_NO_SGR=1 && groff -e -T ascii -man ${CMAKE_CURRENT_SOURCE_DIR}/DAGonFS_P2P.man > DAGonFS_P2P.1
                   DEPENDS DAGonFS_P2P.man
                   COMMENT "Creating man page")
add_custom_target(man ALL
                  DEPENDS DAGonFS_P2P.1)

add_custom_command(OUTPUT DAGonFS_P2P.txt
                   COMMAND ul -t dumb DAGonFS_P2P.1 > DAGonFS_P2P.txt
                   DEPENDS DAGonFS_P2P.1
                   COMMENT "Creating text file")
add_custom_target(txt ALL
                  DEPENDS DAGonFS_P2P.txt)

add_custom_command(OUTPUT DAGonFS_P2P.pdf
                   COMMAND ps2pdf DAGonFS_P2P.ps > DAGonFS_P2P.pdf
                   DEPENDS DAGonFS_P2P.ps
                   COMMENT "Creating pdf file")
add_custom_target(pdf ALL
                  DEPENDS DAGonFS_P2P.pdf)

install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DAGonFS_P2P.ps" DESTINATION "${DOCDIR}")
install(FILES DAGonFS_P2P.man DESTINATION "${MANDIR}/man1" RENAME DAGonFS_P2P.1)
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DAGonFS_P2P.txt" DESTINATION "${DOCDIR}")
install(FILES "${CMAKE_CURRENT_BINARY_DIR}/DAGonFS_P2P.pdf" DESTINATION "${DOCDIR}")
