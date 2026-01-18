#!/usr/bin/env bash
# Internal core for JMTCS. This file is meant to be executed, not sourced.

print_info() {
    echo "[INFO] $1"
    echo "JMTCS - Coupling of CFD and neutronic codes"
    echo "Author: Tomas Korinek"
    echo "Version: 1.0.0"
}

print_usage() {
    echo "Usage: $1 [options]"
    echo "Options:"
    echo "  -h, --help          Show this help message and exit"
    echo "  -i, --info          Show script information and exit"
    echo "  -v, --version       Show version information and exit"
    echo "  -t, --test          Run test function"
    echo "  -I, --interactive   Run in interactive placeholder mode"
}

print_version() {
    echo "$1"
    echo "JMTCS version 1.0.0"
}

interactive_mode() {
    if command -v gnuplot &> /dev/null
    then
        #gnuplot -e "log='log'" plot - &> /dev/null &
        gnuplot -e "log='log.gnuplot'" plot - &> /dev/null &
    else 
        echo "gnuplot could not be found"
    fi
}

copy_results() {
    cp $OPENFOAM_FOLDER/log* $i/
    for of_region in ${FUEL_OPENFOAM[@]}; do
        cp $OPENFOAM_FOLDER/constant/$of_region/volPower $i/
    done
    # data copying
    cp ${SERPENT_INPUT}_mesh* $i/
    cp ${SERPENT_INPUT}_geom* $i/
    cp log.serpent $i/
}

copy_ifc_files()  {
    # Copy ifc files
    for file in ${FUEL_IFC[@]}; do
        #chek if file exists
        f_orig="${file}.orig"
        if [ ! -f  $f_orig ]; then
            echo "File $file.orig not found"
            exit 1
        fi
        cp $file.orig $file
    done
    for file in ${COOLANT_IFC[@]}; do
        cp $file.orig $file
    done
}


copy_input_file() {
    cp $SERPENT_INPUT.orig $SERPENT_INPUT
}

power_relaxation() {
    # Run power relaxation
    for of_region in ${FUEL_OPENFOAM[@]}; do
        
        cp constant/$of_region/volPower $latestTime/$of_region/volPower
        cp constant/$of_region/volPower $latestTime/$of_region/volPower_S
        
        if (($i == 1)); 
        then
            cp constant/$of_region/volPower $latestTime/$of_region/volPower_0
        else
            secondTime=$(foamListTimes -withZero -latestTime | tail -n 2 | head -n 1)
            cp $secondTime/$of_region/volPower $latestTime/$of_region/volPower_0
        fi
        
        # Volumetric power manipulation
        sed -i "s/volPower/volPower_0/" $latestTime/$of_region/volPower_0
        sed -i "s/volPower/volPower_S/" $latestTime/$of_region/volPower_S
        # Execute power relaxation
        powerRelax -alphaRelax $currentAlpha -region $of_region > log.powerRelax -latestTime
        # Copy relaxed power to constant folder
        cp $latestTime/$of_region/volPower constant/$of_region/
    done
}

read_settings() {
    # Read settings from file settings.sh
    
    # check if settings.sh exists
    if [ ! -f settings.sh ]; then
        echo "settings.sh not found"
        exit 1
    else
        echo "Reading settings from settings.sh"
    fi

    source settings.sh
    echo ""
    echo "Checking settings"
    # check if N_LOOPS is set and is greater than 0
    if [ -z $N_LOOPS ] || [ $N_LOOPS -le 0 ]; then
        echo "N_LOOPS is not set or is less than 0"
        exit 1
    fi
    #check if N_PROCS is set and is greater than 0 and lower than number of cores
    if [ -z $N_PROCS ] || [ $N_PROCS -le 0 ] || [ $N_PROCS -gt $(nproc) ]; then
        echo "N_PROCS is not set or is less than 0 or greater than number of cores"
        exit 1
    fi
    # check if N_INIT_NEUTRONS is set and is greater than 0
    if [ $N_INIT_NEUTRONS -le 0 ]; then
        echo "N_INIT_NEUTRONS is not set or is less than 0"
        exit 1
    fi
    # check if OPENFOAM_ITERATIONS is set and is greater than 0
    if [ -z $OPENFOAM_ITERATIONS ] || [ $OPENFOAM_ITERATIONS -le 0 ]; then
        echo "OPENFOAM_ITERATIONS is not set or is less than 0"
        exit 1
    fi
    # check if serpent executable exists
    if [ ! -f $SERPENT ]; then
        echo "SERPENT executable not found or is not set"
        exit 1
    else
        echo "Serpent executable found"
    fi
    SERPENT_ORIG="${SERPENT_INPUT}.orig"
    # check if serpent input file exists
    if [ ! -f $SERPENT_ORIG ]; then
        echo "SERPENT_INPUT file not found or is not set"
        exit 1
    else
        echo "Serpent input file found"
    fi
    # check if OpenFOAM bashrc exists
    if [ ! -f $OPENFOAM_SOURCE ]; then
        echo "OpenFOAM bashr not found or is not set"
        exit 1
    else
        echo "OpenFOAM bashrc found"
    fi
    # check if OpenFOAM case exists
    if [ ! -d $OPENFOAM_FOLDER ]; then
        echo "OPENFOAM_FOLDER case not found"
        exit 1
    else
        echo "OpenFOAM case found"
    fi
    echo
    echo "Settings read successfull"
    echo ""

}

main() {
    # Parse command line arguments
    interactive_mode_placeholder=false
    test_mode=false

    # Convert long options to short for getopts
    new_args=()
    for arg in "$@"; do
        case "$arg" in
            --help) new_args+=("-h") ;;
            --info) new_args+=("-i") ;;
            --version) new_args+=("-v") ;;
            --test) new_args+=("-t") ;;
            --interactive) new_args+=("-I") ;;
            *) new_args+=("$arg") ;;
        esac
    done
    set -- "${new_args[@]}"

    OPTIND=1
    while getopts "hvtiabI" opt; do
        case "$opt" in
            h)
                print_usage
                exit 0
                ;;
            i)
                print_info
                exit 0
                ;;
            v)
                print_version
                exit 0
                ;;
            t)
                test_mode=true
                ;;
            I)
                interactive_mode_placeholder=true
                ;;
            ?)
                echo "Unknown option: -$OPTARG"
                print_usage
                exit 1
                ;;
        esac
    done
    shift $((OPTIND -1))
    echo
    echo "******************************************"
    echo "*              Running JMTCS             *"
    echo "******************************************"
    echo
    # Read settings
    read_settings

    # Source the OF bashrc file
    source $OPENFOAM_SOURCE

    echo "Number of Picard iterations: $N_LOOPS"

    # Main loop of JMTCS
    for (( i=1; i<=N_LOOPS; i++)); do
        echo
        echo "******************************************"
	    echo "*             Iteration $i               *"
	    echo "******************************************"
        echo
        echo "$(date)"
        echo
        # Check what is the latest time
        latestTime=$(foamListTimes -case $OPENFOAM_FOLDER -withZero -latestTime | tail -n 1)	
        echo "Latest time: $latestTime"
        # delete and create actual iteration folder
        rm -rf $i
	    mkdir $i

        #--------------
        # Serpent loop
        #--------------
        echo
        echo "Serpent loop"
        echo

        # Calculate value of alpha_actual
        read -r currentAlpha currentNeutrons < <(./alphaSet $i $N_INIT_NEUTRONS)
        echo "Current relaxation factor $currentAlpha and simulated neutrons $currentNeutrons"
        echo
        # Run serpent

        # Copy interface files
        copy_ifc_files

        # Check time in ifc files
        for file in ${FUEL_IFC[@]}; do
            sed -i "s/TIME/$latestTime/" $file
        done
        for file in ${COOLANT_IFC[@]}; do
            sed -i "s/TIME/$latestTime/" $file
        done
        
        # Copy serpent input file
        copy_input_file

        sed -i "s/NEUTRONS/$currentNeutrons/" $SERPENT_INPUT
        if (( i == 1 )); then
            sed -i "s/%FIRST_STEP//" $SERPENT_INPUT
        else
            sed -i "s/%SECOND_STEP//" $SERPENT_INPUT
        fi

        # Start gnuplot if interactive mode is set and it exist
        if $interactive_mode_placeholder; then
            interactive_mode
        fi

        # Run serpent
        {
            $SERPENT $SERPENT_INPUT -omp $N_PROCS > log.serpent
        } || {
            echo
            echo "******************************************"
            echo "*         Serpent stopped working!       *"
            echo "*            check log.serpent           *"
            echo "******************************************"
            exit 1
        }

        # Kill gnuplot if it was started
        if $interactive_mode_placeholder; then
            pkill -x gnuplot &> /dev/null
        fi
        

        #--------------
        # OpenFOAM loop
        #--------------
        echo
        echo
        echo "OpenFOAM loop"
        echo
        echo "$(date)"
        echo
        
        cd $OPENFOAM_FOLDER
        ./isBoundarySet.sh > log.setBoundary

        # Relaxation of power distribution
        power_relaxation
        
        latestTime=$(foamListTimes -latestTime -withZero | tail -n 1)
        endTime=$(($latestTime+$OPENFOAM_ITERATIONS))

        # Time setting
        foamDictionary -entry "endTime" -set "$endTime" system/controlDict > log.foamDictionary
        foamDictionary -entry "stopAt" -set "endTime" system/controlDict  > log.foamDictionary.end

        


        # Decompose and volPower manipulation for processors
        decomposePar -force -allRegions -constant > log.decomposePar

        # Start gnuplot if interactive mode is set and it exist
        if $interactive_mode_placeholder; then
            interactive_mode
        fi
            
        # OpenFOAM main simulation
        {
            foamJob -p -w chtMultiRegionSimpleFoam
        } || {
            echo
            echo "******************************************"
            echo "*        OpenFOAM is not working!        *"
            echo "*             check log in               *"
            echo "* $OPENFOAM_FOLDER      *"
            echo "******************************************"
            exit 1
        }

        # Kill gnuplot if it was started
        if $interactive_mode_placeholder; then
            pkill -x gnuplot &> /dev/null
        fi
        
        # Reconstructing latest time
        reconstructPar -latestTime -allRegions > log.reconstructPar
        latestTime=$(foamListTimes -withZero -latestTime | tail -n 1)

        # save logfile
        saveTime=$(foamListTimes -withZero -latestTime | tail -n 1)
        cd ..

        # Copy results
        copy_results
    done

    echo
    echo "$(date)"
    echo
    echo "Simulation finished"
    echo
}

main "$@"
