
#process for getting script file directory
variable dispScriptFile [file normalize [info script]]
proc getScriptDirectory {} {
    variable dispScriptFile
    set scriptFolder [file dirname $dispScriptFile]
    return $scriptFolder
}

#change working directory to script file directory
cd [getScriptDirectory]
#set ip_repo_path to script dir
set ip_repo_path [getScriptDirectory]

# PACKAGE SVM IP
source $ip_repo_path\/VGA_DMA_IP\/src\/script\/VGA.tcl

# PACKAGE DESKEW IP
source $ip_repo_path\/myLed_IP\/src\/script\/myLed.tcl

# PACKAGE DESKEW IP
source $ip_repo_path\/Top\/Top.tcl
