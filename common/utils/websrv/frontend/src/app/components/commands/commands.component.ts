import { Component } from '@angular/core';
import { FormControl } from '@angular/forms';
import { BehaviorSubject } from 'rxjs';
import { Observable } from 'rxjs/internal/Observable';
import { map } from 'rxjs/internal/operators/map';
import { tap } from 'rxjs/internal/operators/tap';
import { CommandsApi, IOptionType, ISubCommands, IVariable } from 'src/app/api/commands.api';
import { SubCmdCtrl } from 'src/app/controls/cmds.control';
import { InfosCtrl } from 'src/app/controls/infos.control';
import { VariableCtrl } from 'src/app/controls/variable.control';
import { LoadingService } from 'src/app/services/loading.service';


@Component({
  selector: 'app-commands',
  templateUrl: './commands.component.html',
  styleUrls: ['./commands.component.css'],
})
export class CommandsComponent {

  IOptionType = IOptionType;

  infos$: Observable<InfosCtrl>
  variables$ = new BehaviorSubject<VariableCtrl[]>([])
  subcmds$: BehaviorSubject<SubCmdCtrl> | undefined

  selectedCmd = new FormControl()
  selectedSubCmd = new FormControl()
  selectedVariable?: VariableCtrl

  constructor(
    public commandsApi: CommandsApi,
    public loadingService: LoadingService,
  ) {
    this.infos$ = this.commandsApi.readInfos$().pipe(
      map((doc) => new InfosCtrl(doc)),
    );
  }

  onCmdSubmit(subCmdName: string) {
    this.commandsApi.runCommand$(subCmdName).subscribe();
  }


  onVarSubmit(varCtrl: VariableCtrl) {
    this.commandsApi.setVariable$(varCtrl.api()).subscribe();
  }

  onSelect(cmdNameFC: FormControl) {
    this.commandsApi.getOptions$(cmdNameFC.value).subscribe(opts => {
      this.variables$.next(opts.filter(iopt => iopt.type === IOptionType.variable).map(iopt => new VariableCtrl(iopt as IVariable)))
      const [subCmds] = opts.filter(iopt => iopt.type === IOptionType.subcommand)
      this.subcmds$ = new BehaviorSubject<SubCmdCtrl>(new SubCmdCtrl(subCmds as ISubCommands))
    })
  }
}