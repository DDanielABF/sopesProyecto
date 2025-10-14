import { computed, Injectable, signal } from '@angular/core';
import { ResponseLogin } from '../../auth/auth-service/auth-service.service';

@Injectable({
  providedIn: 'root',
})
export class StorageService {
  private readUser(): ResponseLogin | null {
    const raw = localStorage.getItem('user');
    try {
      return raw ? (JSON.parse(raw) as ResponseLogin) : null;
    } catch {
      return null;
    }
  }

  user = signal<ResponseLogin | null>(this.readUser());
  isLoggedIn = computed(() => this.user() != null);

  setUser(u: ResponseLogin) {
    localStorage.setItem('user', JSON.stringify(u));
    this.user.set(u);
  }

  clearUser() {
    localStorage.removeItem('user');
    this.user.set(null);
  }

  deleteAllStorage() {
    localStorage.clear();
    this.user.set(null);
  }
}
