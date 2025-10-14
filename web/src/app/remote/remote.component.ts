import {
  AfterViewInit,
  Component,
  ElementRef,
  OnDestroy,
  ViewChild,
} from '@angular/core';
import {
  concatMap,
  filter,
  fromEvent,
  map,
  sampleTime,
  Subject,
  takeUntil,
} from 'rxjs';
import { RemoteInputService } from '../shared/service/remote-input.service';
import { SharedModule } from '../shared/shared.module';
import { LINUX_KEYCODE } from '../shared/keymap';

@Component({
  selector: 'app-remote',
  imports: [SharedModule],
  templateUrl: './remote.component.html',
  styleUrl: './remote.component.scss',
  standalone: true,
})
export class RemoteComponent implements AfterViewInit, OnDestroy {
  @ViewChild('captureArea', { static: true }) areaRef!: ElementRef<HTMLElement>;
  private destroy$ = new Subject<void>();
  focused = false;

  constructor(private remote: RemoteInputService) {}

  ngAfterViewInit(): void {
    const area = this.areaRef.nativeElement;

    // Movimiento del Mouse
    fromEvent<PointerEvent>(area, 'pointermove')
      .pipe(
        sampleTime(20),
        map((ev) => ({
          dx: Math.trunc(ev.movementX),
          dy: Math.trunc(ev.movementY),
        })),
        filter(({ dx, dy }) => this.focused && (dx !== 0 || dy !== 0)),
        concatMap(({ dx, dy }) => this.remote.move(dx, dy)),
        takeUntil(this.destroy$)
      )
      .subscribe();

    // CLICK IZQ/DER
    fromEvent<PointerEvent>(area, 'pointerdown')
      .pipe(
        concatMap((ev) => {
          if (!this.focused) area.focus();
          ev.preventDefault();
          const btn: 1 | 2 = ev.button === 0 ? 1 : 2;
          return this.remote.click(0, 0, btn);
        }),
        takeUntil(this.destroy$)
      )
      .subscribe();

    fromEvent(area, 'contextmenu')
      .pipe(takeUntil(this.destroy$))
      .subscribe((e: Event) => e.preventDefault());
    fromEvent(area, 'dragstart')
      .pipe(takeUntil(this.destroy$))
      .subscribe((e: Event) => e.preventDefault());

    // Teclado
    fromEvent<KeyboardEvent>(area, 'keydown')
      .pipe(
        filter(() => this.focused),
        map((ev) => {
          const code = ev.code;
          const keycode = LINUX_KEYCODE[code];
          if (keycode) ev.preventDefault();
          return keycode ?? 0;
        }),
        filter((kc) => kc > 0),
        concatMap((kc) => this.remote.sendKey(kc)),
        takeUntil(this.destroy$)
      )
      .subscribe();
  }

  ngOnDestroy(): void {
    this.destroy$.next();
    this.destroy$.complete();
  }
}
