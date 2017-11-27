
/**
   WORKFLOW.SWIFT
*/

import assert;
import io;
import launch;
import string;
import sys;

// Data transfer method: FLEXPATH, DATASPACES, MPI, or BP
string rmethod = argv("s", "FLEXPATH");

void ready;

// Total worker processes in the run
int availproc = turbine_workers();

// Heat Transfer Processes
int htproc_x = 4;
int htproc_y = 3;
int htproc = htproc_x * htproc_y;
// Stage Write Processes
int swproc = 3;
// DataSpaces Processes
int dsproc = 1;

app(void signal) check_conf_exists () {
       "./check_conf_exists.sh"
}

app(void signal) dummy () {
	"true"
}

check_procs(int minimum_proc) {
    assert(availproc >= minimum_proc,
           "Workflow cannot run: Not enough processes assigned. " +
           "Need " + minimum_proc + " worker processes.");
}

if(rmethod == "DATASPACES")
{
    check_procs(htproc + swproc + dsproc);
    program3 = "dataspaces_server";
    arguments3 = split("-s %d -c %d" % (dsproc, (htproc + swproc)), " ");
    printf("swift: launching %s", program3);
    exit_code3 = @par=dsproc launch(program3, arguments3);
    printf("swift: received exit code: %d", exit_code3);
    if(exit_code3 != 0)
    {
        printf("swift: The launched application did not succeed.");
    }
    ready = check_conf_exists();
}
else
{
    check_procs(htproc + swproc);
    ready = dummy();    
}

run_stager () {
    program2 = "stage_write/stage_write";
    arguments2 = split("heat.bp staged.bp %s \"\" MPI \"\"" % rmethod , " ");
    printf("size: %i", size(arguments2));
    printf("swift: launching: %s", program2);
    exit_code2 = @par=swproc launch(program2, arguments2);
    printf("swift: received exit code: %d", exit_code2);
    if (exit_code2 != 0)
    {
        printf("swift: The launched application did not succeed.");
    }
}

wait(ready) {
    program1 = "./heat_transfer_adios2";
    arguments1 = split("heat  %d %d 40 50  6 500" % (htproc_x, htproc_y), " ");
    printf("swift: launching: %s", program1);
    exit_code1 = @par=htproc launch(program1, arguments1);
    printf("swift: received exit code: %d", exit_code1);
    if (exit_code1 != 0)
    {
        printf("swift: The launched application did not succeed.");
    }

    if(rmethod == "BP")
    {
        wait (exit_code1)
        {
            run_stager();
        }
    }
    else
    {
        run_stager();
    }
}

// Local Variables:
// c-basic-offset: 4;
// End:
