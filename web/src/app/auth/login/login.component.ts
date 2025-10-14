import { Component, inject, OnInit, signal } from '@angular/core';
import { SharedModule } from '../../shared/shared.module';
import { FormBuilder, FormGroup, Validators } from '@angular/forms';
import { Router } from '@angular/router';
import { StorageService } from '../../shared/service/storage.service';
import {
  AuthLogin,
  AuthServiceService,
  ResponseLogin,
} from '../auth-service/auth-service.service';

@Component({
  selector: 'app-login',
  imports: [SharedModule],
  templateUrl: './login.component.html',
  styleUrl: './login.component.scss',
})
export class LoginComponent implements OnInit {
  formBuilder = inject(FormBuilder);
  form!: FormGroup;
  hide = signal(true);

  constructor(
    private router: Router,
    private storage: StorageService,
    private authService: AuthServiceService
  ) {}

  ngOnInit() {
    this.createForm();
  }

  createForm() {
    this.form = this.formBuilder.group({
      username: ['', Validators.required],
      password: ['', Validators.required],
    });
  }

  async login() {
    if (this.form.valid) {
      const login: AuthLogin = {
        username: this.form.value.username,
        password: this.form.value.password,
      };

      try {
        const response: any = await this.authService.authLogin(login);
        if (response.ok) {
          this.storage.setUser(response);
          await this.router.navigate(['auth/remote']);
        }
        this.reset();
      } catch (error: any) {
        if (error.status === 401) {
          alert('Usuario o contraseña incorrectos');
        } else {
          alert('Error inesperado: ' + error.message);
        }
        this.reset();
      }
    }
  }

  reset() {
    this.form.reset();
  }

  showPassword(event: MouseEvent) {
    this.hide.set(!this.hide());
    event.stopPropagation();
  }
}
