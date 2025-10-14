import { ChangeDetectionStrategy, Component } from '@angular/core';
import { SharedModule } from '../shared/shared.module';
import { catchError, map, Observable, startWith, switchMap, timer } from 'rxjs';
import {
  Metrics,
  RemoteInputService,
} from '../shared/service/remote-input.service';

@Component({
  selector: 'app-metrics',
  imports: [SharedModule],
  templateUrl: './metrics.component.html',
  styleUrl: './metrics.component.scss',
  standalone: true,
  changeDetection: ChangeDetectionStrategy.OnPush,
})
export class MetricsComponent {
  private lastCpu?: number;
  private lastRam?: number;

  constructor(private api: RemoteInputService) {
    this.vm$.subscribe((vm) => {
      if (vm.error) return;
      this.lastCpu = vm.cpu;
      this.lastRam = vm.ram;
    });
  }

  color(v: number) {
    if (v < 60) return 'ok';
    if (v < 85) return 'warn';
    return 'bad';
  }

  clamp(n: number) {
    if (n == null || isNaN(n)) return 0;
    if (n < 0) return 0;
    if (n > 100) return 100;
    return n;
  }

  vm$: Observable<{ cpu: number; ram: number; error?: string }> = timer(
    0,
    1000
  ).pipe(
    switchMap(() =>
      this.api.getMetrics().pipe(
        map((m: Metrics) => ({
          cpu: this.clamp(m.cpu),
          ram: this.clamp(m.ram),
        })),
        catchError(() => [
          {
            cpu: this.lastCpu ?? 0,
            ram: this.lastRam ?? 0,
            error: 'No se pudo actualizar',
          },
        ])
      )
    ),
    startWith({ cpu: 0, ram: 0 })
  );
}
