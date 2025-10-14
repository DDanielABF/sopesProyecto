import { Injectable } from '@angular/core';
import { HttpClient } from '@angular/common/http';
import { Observable } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class RemoteInputService {
  private base = 'http://localhost:18080';

  constructor(private http: HttpClient) {}

  move(x: number, y: number) {
    const body: MousePayload = { x, y, click: 0 };
    return this.http.post(`${this.base}/mouse`, body, { responseType: 'json' });
  }

  click(x: number, y: number, btn: 1 | 2) {
    const body: MousePayload = { x, y, click: btn };
    return this.http.post(`${this.base}/mouse`, body, { responseType: 'json' });
  }

  sendKey(keycode: number) {
    return this.http.post(`${this.base}/keyboard`, { key: keycode });
  }

  getMetrics(): Observable<Metrics> {
    return this.http.get<Metrics>(`${this.base}/metrics`);
  }
}

interface MousePayload {
  x: number;
  y: number;
  click?: number;
}

export interface Metrics {
  cpu: number;
  ram: number;
  status: 'ok';
}
