from codar.cheetah import Campaign
from codar.cheetah import parameters as p
from codar.cheetah.parameters import SymLink

class HeatTransfer(Campaign):

    name = "heat-transfer"

    codes = [ 
              ("dataspaces", dict(exe="/lustre/atlas/proj-shared/csc249/CSC249ADCD01/software/titan.gnu/gcc-4.9.3/dataspaces-develop/bin/dataspaces_server",
                                  sleep_after=10,
                                  linked_with_sosflow=False)),
              ("stage_write", dict(exe="stage_write/stage_write", sleep_after=5,
                                   linked_with_sosflow=True)),
              ("heat", dict(exe="heat_transfer_adios2", sleep_after=5,
                            linked_with_sosflow=True,
                            adios_xml_file='heat_transfer.xml')),
            ]

    supported_machines = ['local','titan']
    kill_on_partial_failure=True
    sosd_path = "/lustre/atlas/proj-shared/csc249/CSC249ADCD01/software/titan.gnu/gcc-4.9.3/sos_flow/bin/sosd"
    #run_post_process_script = "run-post-process.sh"
    umask = '027'
    
    scheduler_options = {
        "titan": {"project":"CSC249ADCD01",
                  "queue":"batch" }
    }
    
    app_config_scripts = {
        'titan': 'titan_config.sh',
        'local': None,
    }

    sweeps = [
     
     p.SweepGroup(
      "sim",
      #nodes=2,
      #walltime=180,
      per_run_timeout=180,
      sosflow_profiling=False,
      component_subdirs=False,
      #component_inputs={"heat":[SymLink("workflow.swift"), SymLink(
      #    "stage_write"), "dataspaces.conf"]},

      parameter_groups=
       [p.Sweep(
        node_layout={ "titan": [{ "heat": 16}, {"dataspaces_server":1 }] },
        parameters=[
        p.ParamRunner("heat", "nprocs", [16]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [4]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [4]),
        p.ParamCmdLineArg("heat", "xsize", 4, [32]),
        p.ParamCmdLineArg("heat", "ysize", 5, [32]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [10]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [2]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none", "zlib", "bzip2", "sz", "zfp", "mgard:tol=0.00001",
            "blosc:threshold=4096,shuffle=bit,lvl=1,threads=4,compressor=zstd"]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["MPI_AGGREGATE:num_aggregators=2;num_ost=2;have_metadata_file=0"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=2;num_ost=2;have_metadata_file=0;"]),
        ]),
      ]),

     
     p.SweepGroup(
      "staging-dataspaces",
      per_run_timeout=500,
      sosflow_profiling=False,
      #component_inputs={"dataspaces":["dataspaces.conf"],},
     
      parameter_groups=
      [p.Sweep(
        node_layout={ "titan": [{ "dataspaces": 1 }, {"stage_write": 8}, {"heat": 16}] },
        parameters=[
     
     
        p.ParamRunner("stage_write", "nprocs", [4]),
        p.ParamCmdLineArg("stage_write", "input", 1, ["heat_sim_data.bp"]),
        p.ParamCmdLineArg("stage_write", "output", 2, ["staging_output.bp"]),
        p.ParamCmdLineArg("stage_write", "rmethod", 3, ["DIMES"]),
        p.ParamCmdLineArg("stage_write", "ropt", 4, [""]),
        p.ParamCmdLineArg("stage_write", "wmethod", 5, ["POSIX"]),
        p.ParamCmdLineArg("stage_write", "wopt", 6, [""]),
        p.ParamCmdLineArg("stage_write", "variables", 7, ["T"]),
        p.ParamCmdLineArg("stage_write", "transform", 8,
                          ["none","zfp:accuracy=0.001","sz:absolute=0.001","zlib:9","bzip2:9"]),
     
        p.ParamRunner("heat", "nprocs", [16]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [4]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [4]),
        p.ParamCmdLineArg("heat", "xsize", 4, [32]),
        p.ParamCmdLineArg("heat", "ysize", 5, [32]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [4]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [2]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none",]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["DIMES"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=2;num_ost=2;have_metadata_file=0;"]),
        ]),
      ]),
     
     p.SweepGroup(
      "staging-flexpath",
      per_run_timeout=500,
      sosflow_profiling=False,
      #component_inputs={"dataspaces":["dataspaces.conf"],},
     
      parameter_groups=
      [p.Sweep(
        node_layout={ "titan": [{ "dataspaces": 1 }, {"stage_write": 8}, {"heat": 16}] },
        parameters=[
     
     
        p.ParamRunner("stage_write", "nprocs", [4]),
        p.ParamCmdLineArg("stage_write", "input", 1, ["heat_sim_data.bp"]),
        p.ParamCmdLineArg("stage_write", "output", 2, ["staging_output.bp"]),
        p.ParamCmdLineArg("stage_write", "rmethod", 3, ["FLEXPATH"]),
        p.ParamCmdLineArg("stage_write", "ropt", 4, [""]),
        p.ParamCmdLineArg("stage_write", "wmethod", 5, ["POSIX"]),
        p.ParamCmdLineArg("stage_write", "wopt", 6, [""]),
        p.ParamCmdLineArg("stage_write", "variables", 7, ["T"]),
        p.ParamCmdLineArg("stage_write", "transform", 8,
                          ["none","zfp:accuracy=0.001","sz:absolute=0.001","zlib:9","bzip2:9"]),
     
        p.ParamRunner("heat", "nprocs", [16]),
        p.ParamCmdLineArg("heat", "output", 1, ["heat_sim_data"]),
        p.ParamCmdLineArg("heat", "xprocs", 2, [4]),
        p.ParamCmdLineArg("heat", "yprocs", 3, [4]),
        p.ParamCmdLineArg("heat", "xsize", 4, [32]),
        p.ParamCmdLineArg("heat", "ysize", 5, [32]),
        p.ParamCmdLineArg("heat", "timesteps", 6, [4]),
        p.ParamCmdLineArg("heat", "checkpoints", 7, [2]),
        
        p.ParamAdiosXML("heat", "transform_T", "adios_transform:heat:T",
            ["none",]),
        p.ParamAdiosXML("heat", "transport_T", "adios_transport:heat",
            ["FLEXPATH"]),
        p.ParamAdiosXML("heat", "transport_T_final", "adios_transport:heat_final",
            ["MPI_AGGREGATE:num_aggregators=2;num_ost=2;have_metadata_file=0;"]),
        ]),
      ]),
    ]
