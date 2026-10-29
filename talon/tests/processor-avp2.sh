# Processor.exe (AvP2): every sample world of the AvP2 Editing Tools v1.0,
# processed by the shipped exe and by ours (`Processor.exe <world.ed>
# -skipdialog`, the default options), must give byte-identical .dat files.
#
# Each run gets a fresh directory (the processor reads and writes
# processor.opt in its working directory), and both exes run from paths of
# the same length. The physics BSP's T-junction splits write uninitialised
# vertex colours (RETAIL_BUGS.md): they follow the heap, which follows the
# exe's path and the lighting threads' timing (the shipped exe alone, on one
# CPU and on several, differs there), so tests/datcmp.py compares the files
# byte for byte except where those colours hold garbage.

SHIPPED="$REFERENCE/avp2tools/Tools/Processor.exe"
OURS="$TALON/src/LT2/built/Release/tools/Processor/Processor.exe"
WORLDS="$REFERENCE/avp2tools/AVP2/Worlds"
need "$SHIPPED" "make reference"
need "$OURS" "make"

suite_processor_avp2() {
    local dir="$WORK/processor-avp2" w name side
    rm -rf "$dir"
    for side in ship ours; do
        mkdir -p "$dir/$side/bin"
    done
    cp "$SHIPPED" "$dir/ship/bin/Processor.exe"
    cp "$OURS" "$dir/ours/bin/Processor.exe"

    for w in SinglePlayer/Sample_Marine SinglePlayer/Sample_Pred SinglePlayer/Sample_Alien \
             Multi/DM/Sample_DM Multi/Evac/Sample_Evac Multi/Evac/Sample_Evac_Events \
             Multi/Overrun/Sample_OverRun; do
        name=$(basename "$w")
        for side in ship ours; do
            mkdir -p "$dir/$side/$name"
            cp "$WORLDS/$w.ed" "$dir/$side/$name/"
            if ! run_tool 900 "$dir/$side/$name" "$(winpath "$dir/$side/bin/Processor.exe")" \
                    "$(winpath "$dir/$side/$name/$name.ed")" -skipdialog; then
                fail "$name: the $side exe did not finish"
                continue 2
            fi
            [ -f "$dir/$side/$name/$name.dat" ] || { fail "$name: the $side exe wrote no .dat"; continue 2; }
        done
        if verdict=$(python3 -I "$TALON/tests/datcmp.py" "$dir/ship/$name/$name.dat" "$dir/ours/$name/$name.dat"); then
            pass "$name.dat ($(stat -c %s "$dir/ours/$name/$name.dat") bytes): $verdict"
        else
            fail "$name.dat $verdict"
        fi
    done
}
