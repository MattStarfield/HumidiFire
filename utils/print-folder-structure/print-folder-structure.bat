@echo off
Set cmd_folder=%CD%
CD ..
CD ..
tree /a >%cmd_folder%\tree.txt
@echo ```>docs\folder-structure.md
more +3 %cmd_folder%\tree.txt>>docs\folder-structure.md
@echo ```>>docs\folder-structure.md
exit