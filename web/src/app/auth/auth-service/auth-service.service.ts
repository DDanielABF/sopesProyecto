import { HttpClient } from '@angular/common/http';
import { Injectable } from '@angular/core';
import { lastValueFrom } from 'rxjs';

@Injectable({
  providedIn: 'root',
})
export class AuthServiceService {
  private base = 'http://localhost:18080';

  constructor(private http: HttpClient) {}

  async authLogin(jsonLogin: AuthLogin) {
    return lastValueFrom(this.http.post<any>(this.base + '/auth', jsonLogin));
  }
}

export interface AuthLogin {
  username: string;
  password: string;
}

export interface ResponseLogin {
  can_access: boolean;
  ok: boolean;
  required: string;
  role: string;
  username: string;
}
