variable dispScriptFile [file normalize [info script]]

proc getScriptDirectory {} {
    variable dispScriptFile
    set scriptFolder [file dirname $dispScriptFile]
    return $scriptFolder
}

set sdir [getScriptDirectory]
cd [getScriptDirectory]

# KORAK#1: Definisanje direktorijuma u kojima ce biti smesteni projekat i konfiguracioni fajl
set resultDir ..\/..\/result\/myLed
file mkdir $resultDir
create_project pkg_led ..\/..\/result\/myLed -part xc7z010clg400-1 -force


# KORAK#2: Ukljucivanje svih izvornih fajlova u projekat
add_files -norecurse ..\/hdl\/myLed_v1_0_S_AXI.v
add_files -norecurse ..\/hdl\/myLed_v1_0.v
add_files -fileset constrs_1 ..\/xdc\/myLed.xdc
update_compile_order -fileset sources_1

# KORAK#3: Pokretanje procesa sinteze
launch_runs synth_1
wait_on_run synth_1
puts "*****************************************************"
puts "* Sinteza zavrsena! *"
puts "*****************************************************"

# KORAK#4: Pakovanje Jezgra
update_compile_order -fileset sources_1
ipx::package_project -root_dir ..\/..\/ -vendor xilinx.com -library user -taxonomy /UserIP -force

set_property vendor FTN [ipx::current_core]
set_property name myLed [ipx::current_core]
set_property display_name myLed-1.0 [ipx::current_core]
set_property description {ip za pristup LED} [ipx::current_core]
set_property company_url http://www.ftn.uns.ac.rs [ipx::current_core]
set_property vendor_display_name FTN [ipx::current_core]
set_property taxonomy {/UserIP} [ipx::current_core]
set_property supported_families {zynq Production} [ipx::current_core]

set_property core_revision 2 [ipx::current_core]
ipx::create_xgui_files [ipx::current_core]
ipx::update_checksums [ipx::current_core]
ipx::save_core [ipx::current_core]
set_property  ip_repo_paths ..\/..\/ [current_project]
update_ip_catalog
ipx::check_integrity -quiet [ipx::current_core]
ipx::archive_core ..\/..\/myLed_v1_0.zip [ipx::current_core]
close_project
