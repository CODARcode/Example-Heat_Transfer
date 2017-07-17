import assert;
import io;
import launch;
import string;
import sys;

string rmethod;
void ready;
int availproc;

int htproc_x = 4;
int htproc_y = 3;
int htproc = htproc_x * htproc_y;
int swproc = 3;
int dsproc = 1;

availproc = turbine_workers();
rmethod = argv("s", "FLEXPATH");

app(void signal) check_conf_exists () {
       "./check_conf_exists.sh"
}

app(void signal) dummy () {
	"true"
}

if(rmethod == "DATASPACES")
{
    assert(availproc >= (htproc + swproc + dsproc), "Not enough processes assigned. Workflow cannot run.");
    program3 = "dataspaces_server";
    arguments3 = split("-s %d -c %d" % (dsproc, (htproc + swproc)), " ");
    printf("swift: launching %s", program3);
    exit_code3 = @par=dsproc launch(program3, arguments3);
    printf("swift: received exit code: %d", exit_code3);
    if(exit_code3 != 0)
    {
        printf("swift: The launched application did not succed.");
    }
    ready = check_conf_exists();
}
else
{
    assert(availproc >= (htproc + swproc), "Not enough processes assigned. Workflow cannot run.");
    ready = dummy();    
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
